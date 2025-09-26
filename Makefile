# Nom de l'executable
NAME = emu

CC = gcc # Déclaration d'un variable pour y l'utiliser $(CC)
# CFLAGS	= -Wall -Wextra -Werror

SRCDIR = src
INCDIR = includes
LIBDIR = lib
BINDIR = bin
OBJSDIR = obj
SRCS = main.c \
	  cpu8080.c \
	  memory.c \
	  utils.c \
	  io.c \
	  video.c

LIBFLAG = -L $(LIBDIR) -l SDL3 # Permet de compiler SDL

OBJS = $(addprefix $(OBJSDIR)/, $(SRCS:.c=.o)) # addprefix: ajoute le prefixe OBJSDIR/ devant toutes les valuers | $(SRCS:.c=.o): Change toutes les extensions en .o

# Cible: dependance
all: $(NAME)

$(NAME): $(OBJS) | $(BINDIR) # Avec le |, $(OBJSDIR) est une dépendance d’ordre : Make s’assure juste que le dossier existe, mais sa modification ne force pas la recompilation des .o
	gcc -o $(BINDIR)/$@ $^ $(LIBFLAG)

$(OBJSDIR)/%.o: $(SRCDIR)/%.c | $(OBJSDIR)# Toutes les cibles en .o je vais les créer à partir de toutes les dépendances .c
	$(CC) -I $(INCDIR) -c $< -o $@ 
# $< va print la premiere dependance ici vu qu'il y a toujours une dépendance ca sera toujours %c

$(OBJSDIR):
	mkdir $(OBJSDIR)

$(BINDIR):
	mkdir $(BINDIR)

clean:
	del /s /q *.o

fclean: clean
	del /s /q $(BINDIR)\$(NAME).exe

re: fclean $(NAME)

.PHONY:	all clean fclean re
# Variables spéciale
# $@ Nom de la cible
# $< Nom première dépendance
# $^ Liste dépendances
