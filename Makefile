executable = evi

object-files = $(patsubst src/%.c, .intermediate/%.o, $(wildcard src/*.c))
dependencies = $(patsubst %.o, %.d, $(object-files))

all: $(executable)

$(executable): .intermediate $(object-files) 
	$(CC) $(CFLAGS) -o $(executable) $(object-files) -lm

.intermediate:
	mkdir .intermediate

.intermediate/%.o: src/%.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(dependencies)

clean:
	rm -rf .intermediate $(executable)
