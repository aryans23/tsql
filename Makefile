TARGET=Sql
CXX=g++ -g
FLAGS= -Wall 
LIB=
HDRS= Helper.h Query.h QueryPlan.h Algos.h 
SRCS= Helper.cpp Sql.cpp Query.cpp QueryPlan.cpp Algos.cpp 
OBJS= Helper.o Sql.o Query.o QueryPlan.o Algos.o 

$(TARGET):  $(OBJS) StorageManager.o 
	$(CXX) $(FLAGS) $(OBJS) StorageManager.o $(LIB) -o $(TARGET) 

$(OBJS): $(HDRS)

StorageManager.o: Block.h Disk.h Field.h MainMemory.h Relation.h Schema.h SchemaManager.h Tuple.h Config.h
	g++ -c StorageManager.cpp

clean:
	rm $(OBJS) $(TARGET) StorageManager.o
