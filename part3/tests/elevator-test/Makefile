all: consumer producer

consumer: consumer.c wrappers.h
	gcc consumer.c -o consumer.o

producer: producer.c wrappers.h
	gcc producer.c -o producer.o

.PHONY: all run clean

clean:
	rm producer consumer