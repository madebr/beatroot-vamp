
#CXXFLAGS	:= -fPIC -g -DDEBUG_BEATROOT
CXXFLAGS	:= -fPIC -O3 -Wall

OBJECTS	:= BeatRootProcessor.o BeatRootVampPlugin.o Peaks.o Agent.o AgentList.o Induction.o BeatTracker.o

HEADERS := Agent.h AgentList.h BeatRootProcessor.h BeatRootVampPlugin.h BeatTracker.h Event.h Induction.h Peaks.h

beatroot-vamp.so:	$(OBJECTS) $(HEADERS)
	g++ -g -shared $(OBJECTS) -o $@ -Wl,-Bstatic -lvamp-sdk -Wl,-Bdynamic -lpthread -Wl,--version-script=vamp-plugin.map

clean:	
	rm *.o

# DO NOT DELETE

Agent.o: Agent.h Event.h BeatTracker.h AgentList.h Induction.h
AgentList.o: AgentList.h Agent.h Event.h
BeatRootProcessor.o: BeatRootProcessor.h Peaks.h Event.h BeatTracker.h
BeatRootProcessor.o: Agent.h AgentList.h Induction.h
BeatRootVampPlugin.o: BeatRootVampPlugin.h BeatRootProcessor.h Peaks.h
BeatRootVampPlugin.o: Event.h BeatTracker.h Agent.h AgentList.h Induction.h
BeatTracker.o: BeatTracker.h Event.h Agent.h AgentList.h Induction.h
Induction.o: Induction.h Agent.h Event.h AgentList.h
Peaks.o: Peaks.h
Agent.o: Event.h
AgentList.o: Agent.h Event.h
BeatRootProcessor.o: Peaks.h Event.h BeatTracker.h Agent.h AgentList.h
BeatRootProcessor.o: Induction.h
BeatTracker.o: Event.h Agent.h AgentList.h Induction.h
Induction.o: Agent.h Event.h AgentList.h
