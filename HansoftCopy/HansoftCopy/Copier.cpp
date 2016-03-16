//
//  Copier.cpp
//  HansoftCopy
//
//  Created by Patrik Hartlén on 28/11/15.
//  Copyright © 2015 Patrik Hartlén. All rights reserved.
//

#include "Copier.h"
#include "HPMSdkCpp.h"

#ifdef _MSC_VER
#include <tchar.h>
#include <conio.h>
#include <windows.h>
#else
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#if defined(__GNUC__)
#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#endif
#endif
#include <string>
#include <iostream>
#include <map>

using namespace std;
using namespace HPMSdk;

#ifdef __GNUC__

struct CProcessSemaphore
{
    pthread_cond_t m_Cond;
    pthread_mutex_t m_Mutex;
    int m_Counter;
};

#endif

class HansoftColumnCopier : public HPMSdkCallbacks
{
public:

    HPMSdkSession *session_;
    bool broken_connection_;

    virtual void On_ProcessError(EHPMError _Error) override
    {
        HPMString sdk_error = HPMSdkSession::ErrorToStr(_Error);
        wstring Error(sdk_error.begin(), sdk_error.end());

        wcout << "On_ProcessError: " << Error << "\r\n";
        broken_connection_ = true;
    }

    HPMUInt64 next_connection_attempt_;
#ifdef _MSC_VER
    HANDLE process_semaphore_;
#elif __GNUC__
    CProcessSemaphore process_semaphore_;
#endif
    HPMNeedSessionProcessCallbackInfo process_callback_info_;

    HPMString server_;
    int port_;
    HPMString database_;
    HPMString username_;
    HPMString password_;
    HPMString project_;
    HPMString source_;
    HPMString destination_;

    HansoftColumnCopier(string server_, int port_, string database_, string username_, string password_, string project_, string source_, string destination_) :
        server_(server_), port_(port_), database_(database_), username_(username_), password_(password_), project_(project_), source_(source_), destination_(destination_)
    {
        session_ = NULL;
        next_connection_attempt_ = 0;
        broken_connection_ = false;
        // Create the event that will be signaled when the SDK needs processing.
#ifdef _MSC_VER
        process_semaphore_ = CreateSemaphore(NULL, 0, 1, NULL); // Behaves like an auto reset event.
#elif __GNUC__
        pthread_cond_init(&process_semaphore_.m_Cond, NULL);
        pthread_mutex_init(&process_semaphore_.m_Mutex, NULL);
        process_semaphore_.m_Counter = 1;
#endif
    }

    ~HansoftColumnCopier()
    {
#ifdef _MSC_VER
        if (process_semaphore_)
            CloseHandle(process_semaphore_);
#elif __GNUC__
        pthread_mutex_destroy(&process_semaphore_.m_Mutex);
        pthread_cond_destroy(&process_semaphore_.m_Cond);
#endif

        if (session_)
        {
            DestroyConnection();
        }
    }

#ifdef __GNUC__
    bool SemWait(int _timeout)
    {
        time_t now;
        time(&now);

        timespec ts;
        ts.tv_sec = now;
        ts.tv_nsec = _timeout * 1000000;

        pthread_mutex_lock(&process_semaphore_.m_Mutex);
        while (process_semaphore_.m_Counter == 0)
        {
            if (pthread_cond_timedwait(&process_semaphore_.m_Cond, &process_semaphore_.m_Mutex, &ts) == ETIMEDOUT)
            {
                pthread_mutex_unlock(&process_semaphore_.m_Mutex);
                return true;
            }
        }
        --process_semaphore_.m_Counter;
        pthread_mutex_unlock(&process_semaphore_.m_Mutex);
        return true;
    }
#endif

    HPMUInt64 GetTimeSince1970()
    {
#ifdef _MSC_VER
        FILETIME time;
        GetSystemTimeAsFileTime(&time);

        return (HPMUInt64)((((ULARGE_INTEGER &)time).QuadPart / 10) - 11644473600000000);
#else
        timeval time;
        gettimeofday(&time, NULL);
        return (HPMUInt64)time.tv_sec * 1000000;
#endif
    }

    static void NeedSessionProcessCallback(void *_sempahore)
    {
#ifdef _MSC_VER
        ReleaseSemaphore(_semaphore, 1, NULL);
#elif __GNUC__
        CProcessSemaphore *process_semaphore = (CProcessSemaphore *)_sempahore;
        pthread_mutex_lock(&process_semaphore->m_Mutex);
        ++process_semaphore->m_Counter;
        pthread_cond_signal(&process_semaphore->m_Cond);
        pthread_mutex_unlock(&process_semaphore->m_Mutex);
#endif
    }

    bool InitConnection()
    {
        if (session_)
            return true;

        HPMUInt64 current_time = GetTimeSince1970();
        if (current_time > next_connection_attempt_)
        {
            next_connection_attempt_ = 0;

            EHPMSdkDebugMode debug_mode = EHPMSdkDebugMode_Off;

#ifdef _MSC_VER
            process_callback_info_.m_pContext = m_ProcessSemaphore;
#elif __GNUC__
            process_callback_info_.m_pContext = &process_semaphore_;
#endif
            process_callback_info_.m_pCallback = &HansoftColumnCopier::NeedSessionProcessCallback;

            try
            {
                session_ = HPMSdkSession::SessionOpen(server_, port_, database_, username_, password_, this, &process_callback_info_, true, debug_mode, NULL, 0, hpm_str(""), HPMSystemString(), NULL);
            }
            catch (const HPMSdkException &_error)
            {
                HPMString sdk_error = _error.GetAsString();
                wstring Error(sdk_error.begin(), sdk_error.end());
                wcout << hpm_str("SessionOpen failed with error:") << Error << hpm_str("\r\n");
                return false;
            }
            catch (const HPMSdkCppException &_error)
            {
                wcout << hpm_str("SessionOpen failed with error:") << _error.what() << hpm_str("\r\n");
                return false;
            }

            wcout << "Successfully opened session.\r\n";
            broken_connection_ = false;

            return true;
        }

        return false;
    }

    void DestroyConnection()
    {
        if (session_)
        {
            HPMSdkSession::SessionDestroy(session_);
            session_ = NULL;
        }
    }

    using HPMSdkCallbacks::On_Callback;

    void On_Callback(const HPMChangeCallbackData_DashboardChartReceive &_Data) override
    {
    }

    HPMUniqueID FindProjectByName(HPMString name)
    {
        HPMProjectEnum projects = session_->ProjectEnum();
        for (HPMUniqueID project_id : projects.m_Projects)
        {
            HPMProjectProperties properties = session_->ProjectGetProperties(project_id);
            if (properties.m_Name == name)
            {
                wcout << "Found project: " << name.c_str() << "\r\n";
                return project_id;
            }
        }
        wcerr << "Can not find project: " << name.c_str() << "\r\n";
        return HPMUniqueID();
    }

    HPMUniqueID GetProductBacklog(HPMUniqueID project_id)
    {
        auto program_backlog_id = session_->ProjectOpenBacklogProject(project_id);
        if (!program_backlog_id.IsValid())
            wcerr << "Can not open program backlog!\r\n";
        return program_backlog_id;
    }

    HPMUniqueID GetProductSchedule(HPMUniqueID project_id)
    {
        auto program_backlog_id = session_->ProjectOpenBacklogProject(project_id);
        if (!program_backlog_id.IsValid())
            wcerr << "Can not open program schedule!\r\n";
        return program_backlog_id;
    }

    HPMProjectCustomColumnsColumn FindColumn(HPMUniqueID project_id, HPMString name)
    {
        for (auto column : session_->ProjectCustomColumnsGet(project_id).m_ShowingColumns)
        {
            if (name == column.m_Name)
            {
                wcout << "Found showing column: " << name.c_str() << "\r\n";
                return column;
            }
        }
        for (auto column : session_->ProjectCustomColumnsGet(project_id).m_HiddenColumns)
        {
            if (name == column.m_Name)
            {
                wcout << "Found hidden column: " << name.c_str() << "\r\n";
                return column;
            }
        }
        wcerr << "Can not find column: " << name.c_str() << "\r\n";
        return HPMProjectCustomColumnsColumn();
    }

    HPMString GetCustomColumnValue(HPMUniqueID task, HPMProjectCustomColumnsColumn column)
    {
        HPMString value;
        switch (column.m_Type)
        {
            case EHPMProjectCustomColumnsColumnType::EHPMProjectCustomColumnsColumnType_DropList:
            {
                value = session_->TaskGetCustomColumnData(task, column.m_Hash);
                if (value != "")
                {
                    HPMUInt32 id = stoi(value);
                    for (auto droplist_item : column.m_DropListItems)
                    {
                        if (id == droplist_item.m_Id)
                        {
                            value = droplist_item.m_Name;
                            break;
                        }
                    }
                }
                break;
            }
            default:
                value = session_->TaskGetCustomColumnData(task, column.m_Hash);
        }
        return value;
    }

    void SetCustomColumnValue(HPMUniqueID task, HPMProjectCustomColumnsColumn column, HPMString value)
    {
        switch (column.m_Type)
        {
            case EHPMProjectCustomColumnsColumnType::EHPMProjectCustomColumnsColumnType_DropList:
            {
                for (auto droplist_item : column.m_DropListItems)
                {
                    if (droplist_item.m_Name == value)
                    {
                        session_->TaskSetCustomColumnData(task, column.m_Hash, std::to_string(droplist_item.m_Id), false);
                        break;
                    }
                }
                break;
            }
            default:
                session_->TaskSetCustomColumnData(task, column.m_Hash, value, false);
        }
    }

    void CustomColumnCopy()
    {
        auto project_id = FindProjectByName(project_);
        if (!project_id.IsValid())
            return;
        auto program_backlog_id = GetProductBacklog(project_id);
        auto program_schedule_id = GetProductSchedule(project_id);
        if (!program_backlog_id.IsValid())
            return;
        auto source_column = FindColumn(program_backlog_id, source_);
        auto destination_column = FindColumn(program_backlog_id, destination_);
        if (source_column.m_Name == "" || destination_column.m_Name == "")
            return;
        int count = 0;
        for (auto task : session_->TaskEnum(program_backlog_id).m_Tasks)
        {
            auto data = GetCustomColumnValue(task, source_column);
            SetCustomColumnValue(task, destination_column, data);
            count++;
        }
        wcout << "Copied: " << count << " tasks.\r\n";
    }

    void PriorityCopy()
    {
        auto project_id = FindProjectByName(project_);
        if (!project_id.IsValid())
            return;
        auto program_backlog_id = GetProductBacklog(project_id);
        if (!program_backlog_id.IsValid())
            return;
        auto destination_column = FindColumn(program_backlog_id, destination_);
        if (destination_column.m_Name == "")
            return;
        int count = 0;
        HPMUniqueID first;
        map<HPMUniqueID, HPMUniqueID> dictionary;
        for (auto task : session_->TaskRefUtilEnumChildren(program_backlog_id, true).m_Tasks)
        {
            auto previous = session_->TaskRefGetPreviousWorkPriorityID(task);
            dictionary[previous] = task;
            if (previous.m_ID == -2)
                first = task;
            count++;
        }
        if (dictionary.size() == 0)
        {
            wcout << "No items in backlog, nothing to do.\r\n";
            return;
        }
        vector<HPMUniqueID> sorted;
        auto item = first;
        while (dictionary.find(item) != dictionary.end())
        {
            sorted.push_back(item);
            item = dictionary[item];
        }
        if (dictionary.size() > 1)
            sorted.push_back(item);
        int index = 1;
        for (auto task_ref : sorted)
        {
            HPMUniqueID task = session_->TaskRefGetTask(task_ref);
            session_->TaskSetCustomColumnData(task, destination_column.m_Hash, to_string(index++), false);
        }
        wcout << "Copied: " << count << " tasks.\r\n";
    }

    void Copy()
    {
        if (InitConnection())
        {
            try
            {
                if (broken_connection_)
                {
                    DestroyConnection();
                }
                else
                {
                    auto lower(source_);
                    transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower == "priority")
                        PriorityCopy();
                    else
                        CustomColumnCopy();
                }
            }
            catch (HPMSdkException &_error)
            {
                HPMString sdk_error = _error.GetAsString();
                wstring Error(sdk_error.begin(), sdk_error.end());
                wcout << hpm_str("Exception in processing loop: ") << Error << hpm_str("\r\n");
            }
            catch (HPMSdkCppException _error)
            {
                wcout << hpm_str("Exception in processing loop: ") << _error.what() << hpm_str("\r\n");
            }
        }
    }

    int Run()
    {
        Copy();
#ifdef _MSC_VER
        WaitForSingleObjectEx(process_semaphore_, 100, true);
#elif __GNUC__
        SemWait(100);
#endif
        DestroyConnection();

        return 0;
    }
};

void Copy(string server, int port, string database, string username, string password, string project, string source, string destination)
{
    HansoftColumnCopier copier(server, port, database, username, password, project, source, destination);
    copier.Run();
}
