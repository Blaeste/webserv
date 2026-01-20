# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 10:08:04 by eschwart          #+#    #+#              #
#    Updated: 2026/01/20 13:01:35 by eschwart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

MK_DIR := mk

#include ${MK_DIR}/config.mk
#include ${MK_DIR}/dev.mk
include ${MK_DIR}/format.mk
include ${MK_DIR}/git.mk
#include ${MK_DIR}/libs.mk
#include ${MK_DIR}/sources.mk
#include ${MK_DIR}/targets.mk

.DEFAULT_GOAL := all



# ============================================================================ #
#                                CONFIGURATION                                 #
# ============================================================================ #

.SILENT:
.PHONY: all clean fclean re

# Executable name
NAME = webserv

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98
INCLUDES = -I includes

# ============================================================================ #
#                               SOURCE FILES                                   #
# ============================================================================ #

# Source files by category
CGI_FILES = CGI.cpp
CONFIG_FILES = Config.cpp Location.cpp ServerConfig.cpp
HTTP_FILES = HttpRequest.cpp HttpResponse.cpp
SERVER_FILES = Client.cpp Server.cpp Router.cpp
UTILS_FILES = MimeTypes.cpp utils.cpp Logger.cpp

# Add directory prefixes to source files
CGI = $(addprefix srcs/cgi/, $(CGI_FILES))
CONFIG = $(addprefix srcs/config/, $(CONFIG_FILES))
HTTP = $(addprefix srcs/http/, $(HTTP_FILES))
SERVER = $(addprefix srcs/server/, $(SERVER_FILES))
UTILS = $(addprefix srcs/utils/, $(UTILS_FILES))

# All source files
SRCS = srcs/main.cpp $(CGI) $(CONFIG) $(HTTP) $(SERVER) $(UTILS)

# Object files (mirror source structure in obj/ directory)
OBJS = $(SRCS:srcs/%.cpp=obj/%.o)

# ============================================================================ #
#                                  RULES                                       #
# ============================================================================ #

# Default rule: build the executable
all: $(NAME)

# Link object files into executable
$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)
	echo "✓ $(NAME) compiled successfully"

# Compile source files into object files
obj/%.o: srcs/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Remove object files
clean:
	rm -rf obj/
	echo "✓ Object files removed"

# Remove object files and executable
fclean: clean
	rm -f $(NAME)
	echo "✓ Executable removed"

# Rebuild everything from scratch
re: fclean all

test: re
	-pkill webserv || true
	gnome-terminal -- bash -c './webserv config/default.conf; exec bash' &
	sleep 1
	python3 webServeTester.py

eval:
	@test -f tester || wget -q https://cdn.intra.42.fr/document/document/44506/tester
	@test -f cgi_tester || wget -q https://cdn.intra.42.fr/document/document/44507/cgi_tester
	@chmod +x tester cgi_tester
	@mkdir -p YoupiBanane/nop
	@mkdir -p YoupiBanane/Yeah
	@touch YoupiBanane/youpi.bad_extension
	@touch YoupiBanane/youpi.bla
	@touch YoupiBanane/nop/youpi.bad_extension
	@touch YoupiBanane/nop/other.pouic
	@touch YoupiBanane/Yeah/not_happy.bad_extension
	@-pkill webserv 2>/dev/null || true
	@gnome-terminal -- bash -c './webserv config/default.conf; exec bash' &
	@sleep 1
	./tester http://localhost:8080

clear_eval:
	@rm -rf YoupiBanane
	@rm -f tester cgi_tester
