
CXXFLAGS	:= -g

beatroot-vamp.so:	BeatRootProcessor.o BeatRootVampPlugin.o BeatTracker.o Peaks.o Agent.o
	g++ -shared $^ -o $@ -Wl,-Bstatic -lvamp-sdk -Wl,-Bdynamic -lpthread -Wl,--version-script=vamp-plugin.map

clean:	
	rm *.o

