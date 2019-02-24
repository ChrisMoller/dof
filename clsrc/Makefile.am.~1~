bin_PROGRAMS = clc
AM_YFLAGS = -d -v
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c
clc_SOURCES = main.c \
              opsdata.h node.h \
              parser.y lexer.l \
              printext.c printext.h \
              eval.c eval.h \
              vector.c vector.h \
              memory.c memory.h \
              rgb.c rgb.h \
              set.c set.h \
              plot.c plot.h

DISTCLEANFILES = .deps/*.P

AUTOMAKE_OPTIONS = dejagnu

$(bin_PROGRAMS)-eval.o: eval.c opsdata.h printext.h node.h

clc_CFLAGS = -std=c99 -Wall -Werror $(GSL_CFLAGS) $(PLPLOT_CFLAGS) \
     $(LIBFFI_CFLAGS) $(HAVE_DISPLAY)

clc_LDFLAGS = $(GSL_LIBS) $(PLPLOT_LIBS) $(LIBFFI_LIBS) -lm -lreadline

# -leak-check=full
#  algrind --tool=memcheck --leak-check=full --leak-resolution=high -
#                -num-callers=20 --log-file=vgdump ./gfig
#  
#VALGRIND_FLAGS = --gen-suppressions=all --run-libc-freeres=yes \
#    --suppressions=valgrind.supp
#VALGRIND_FLAGS = --gen-suppressions=all --run-libc-freeres=yes

valgrind.log : clc
	valgrind $(VALGRIND_FLAGS) --log-file=$@ ./clc

clean-local: clean-local-check
.PHONY: clean-local-check
clean-local-check:
	rm -rf clc-parse.c clc-parse.h clc-lex.c
