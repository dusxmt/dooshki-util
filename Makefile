CC	= gcc
CFLAGS	= -std=c89 -pedantic -Wall -Wextra -W
LDFLAGS	=
LDLIBS	= 

PROG	= dooshki_args_demo
SRCS	= dooshki_args.c dooshki_args_demo.c
HDRS	= dooshki_args.h

OBJS = $(SRCS:.c=.o)

.PHONY: all clean

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(PROG) $(OBJS)
