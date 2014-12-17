
PACKS := `pkg-config --cflags --libs gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0`

CFLAGS = -std=c++11

LDFLAGS := -lrt -lGL -lGLU -lGLEW -lX11 -lpthread 
LDFLAGS += -ljack

GLIBS = gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0
GTKFLAGS = `pkg-config --cflags $(GLIBS) `
GTKLD := `pkg-config --libs $(GLIBS) `

OBJS := main.o jack.o

all: overlei

%.o: %.cpp 
	g++ -c -g $(CFLAGS) $(GTKFLAGS) $< -o $@

overlei: $(OBJS)
	g++ -g -o $@ $(GTKLD) $(LDFLAGS) $(OBJS)

clean:
	rm -rf overlei
	rm -rf *.o