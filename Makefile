CC=gcc
PARENT=oss
CHILD=child
BINDIR=bin
OBJDIR=obj
INCDIR=include
SRCDIR=src
SRCS:=$(notdir $(shell find $(SRCDIR) -name '*.c'))
OBJS:=$(SRCS:%.c=%.o)

# D_GNU_SOURCE so we can use asprintf, get_current_dir_name, etc.

CFLAGS=-D_GNU_SOURCE -I$(INCDIR)
LDFLAGS=-pthread -lrt
VPATH=$(SRCDIR)

.PHONY: clean debug release parent child

release: child

child: parent
	$(CC) $(patsubst %.o,$(OBJDIR)/%.o,$(filter-out $(PARENT).o,$(OBJS))) -o $(BINDIR)/$(CHILD) $(LDFLAGS)

parent: $(OBJS)
	$(CC) $(patsubst %.o,$(OBJDIR)/%.o,$(filter-out $(CHILD).o,$(OBJS))) -o $(BINDIR)/$(PARENT) $(LDFLAGS)

debug: CFLAGS+=-g -D DEBUG
debug: release

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $(OBJDIR)/$@

clean:
	rm $(OBJDIR)/*.o $(BINDIR)/$(PARENT) $(BINDIR)/$(CHILD)
