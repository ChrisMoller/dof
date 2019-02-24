bin_PROGRAMS = dof
dof_SOURCES = dof.c \
              coc-table.c coc-table.h

DISTCLEANFILES = .deps/*.P

dof_CFLAGS = -std=c99 -Wall -Werror 

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -rf clc-parse.c clc-parse.h clc-lex.c
