SRCDIR = src
OBJDIR = obj
BINDIR = bin

DIRS = $(OBJDIR) \
	   $(BINDIR) \
	   $(OBJDIR)/events \
	   $(OBJDIR)/graphics \
	   $(OBJDIR)/managers \
	   $(OBJDIR)/network \
	   $(OBJDIR)/server \
	   $(OBJDIR)/statistics \
	   $(OBJDIR)/world

UBUNTU_LIBS = -lXxf86vm -lXrandr -lXi
CLIENT_LIBS = -lboost_system -lGL -lGLEW -lSDL2 -lX11 -pthread $(UBUNTU_LIBS)
SERVER_LIBS = -lboost_system -pthread $(UBUNTU_LIBS)


CC = g++ -g

CFLAGS = -Wall -std=c++11

CLIENT_TGT = below
SERVER_TGT = belowServer

TGTDIR = .


COMMON_OBJS=\
	$(OBJDIR)/events/event.o \
	$(OBJDIR)/events/eventDispatcher.o \
	$(OBJDIR)/events/eventFactory.o \
	$(OBJDIR)/events/eventQueue.o \
	$(OBJDIR)/network/serializable.o \
	$(OBJDIR)/world/entity.o \
	$(OBJDIR)/world/worldNode.o \
	$(OBJDIR)/statistics/executionTimer.o \
	$(OBJDIR)/gameState.o \
	$(OBJDIR)/logger.o \
	$(OBJDIR)/smooth.o \
	$(OBJDIR)/task.o \
	$(OBJDIR)/taskQueue.o \
	$(OBJDIR)/threadPool.o

CLIENT_OBJS=\
	$(COMMON_OBJS) \
	$(OBJDIR)/graphics/shader.o \
	$(OBJDIR)/graphics/mesh.o \
	$(OBJDIR)/graphics/obj.o \
	$(OBJDIR)/graphics/shaderProgram.o \
	$(OBJDIR)/network/serverConnection.o \
	$(OBJDIR)/managers/shaderProgramManager.o \
	$(OBJDIR)/managers/clientObjectManager.o \
	$(OBJDIR)/clientGameState.o \
	$(OBJDIR)/main.o

SERVER_OBJS=\
	$(COMMON_OBJS) \
	$(OBJDIR)/network/server.o \
	$(OBJDIR)/managers/serverObjectManager.o \
	$(OBJDIR)/server/serverGameState.o \
	$(OBJDIR)/server/main.o


all: $(TGTDIR)/$(CLIENT_TGT) $(TGTDIR)/$(SERVER_TGT)
client: $(TGTDIR)/$(CLIENT_TGT)
server: $(TGTDIR)/$(SERVER_TGT)




$(TGTDIR)/$(CLIENT_TGT): $(DIRS) $(BINDIR)/$(CLIENT_TGT)
	cp $(BINDIR)/$(CLIENT_TGT) $(TGTDIR)/$(CLIENT_TGT)
	@echo "$@ up to date"

$(TGTDIR)/$(SERVER_TGT): $(DIRS) $(BINDIR)/$(SERVER_TGT)
	cp $(BINDIR)/$(SERVER_TGT) $(TGTDIR)/$(SERVER_TGT)
	@echo "$@ up to date"

$(BINDIR)/$(CLIENT_TGT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(CLIENT_LIBS)

$(BINDIR)/$(SERVER_TGT): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(SERVER_LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cc
	$(CC) $(CFLAGS) -c -o $@ $?

$(OBJDIR)/*/%.o: $(SRCDIR)/*/%.cc
	$(CC) $(CFLAGS) -c -o $@ $?

$(DIRS):
	python mkdir.py $(DIRS)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(BINDIR)
	rm -rf $(TGTDIR)/$(CLIENT_TGT)
	rm -rf $(TGTDIR)/$(SERVER_TGT)

fresh: clean all

