SYSTEM = ./system
UI = ./ui
WEB_SERVER = ./web_server

INCLUDES = -I$(SYSTEM) -I$(UI) -I$(WEB_SERVER)

OBJ = -o ./obj/$@

objects = main.o system_server.o web_server.o input.o gui.o

all: $(objects)
	gcc -o toy_system ./obj/main.o ./obj/system_server.o ./obj/web_server.o ./obj/input.o ./obj/gui.o

main.o:
	gcc $(OBJ) $(INCLUDES) -c main.c

system_server.o:
	gcc $(OBJ) $(INCLUDES) -c ./system/system_server.c

web_server.o:
	gcc $(OBJ) $(INCLUDES) -c ./web_server/web_server.c

input.o:
	gcc $(OBJ) $(INCLUDES) -c ./ui/input.c

gui.o:
	gcc $(OBJ) $(INCLUDES) -c ./ui/gui.c
