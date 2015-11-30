headerfiles := Copier.h OptionParser.h
objectfiles := Copier.o OptionParser.o main.o ../HansoftSDK/HPMSdkCpp.o


DashboardExporter: $(objectfiles)
	$(CXX) $(objectfiles) ../HansoftSDK/Linux2.6/HPMSdk.x64.so -ldl -lpthread -lstdc++ -o HansoftCopy


%.o: %.cpp $(headerfiles)
	$(CXX) -std=c++11 -I ../HansoftSDK -c $*.cpp -o $*.o


# remove compilation products
clean:
	rm -f HansoftCopy *.o
