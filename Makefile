# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 10:08:04 by eschwart          #+#    #+#              #
#    Updated: 2026/03/17 10:55:24 by gdosch           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

MK_DIR := mk

include ${MK_DIR}/format.mk
include ${MK_DIR}/git.mk

.DEFAULT_GOAL := all

# ============================================================================ #
#                                CONFIGURATION                                 #
# ============================================================================ #

.SILENT:
.ONESHELL:
SHELL = /bin/bash
.PHONY: all clean fclean re ensure-xterm run kill test eval

# Executable name
NAME = webserv

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -O3 -std=c++98

# Compile multi-cpu (linux only comment it on other system)
ifeq ($(MAKELEVEL),0)
MAKEFLAGS += -j$(shell nproc)
endif

# ============================================================================ #
#                               SOURCE FILES                                   #
# ============================================================================ #

# Source files by category
CGI_FILES = Cgi.cpp
CONFIG_FILES = ConfigParser.cpp Location.cpp ServerBlock.cpp
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
SRCS = srcs/webserv.cpp $(CGI) $(CONFIG) $(HTTP) $(SERVER) $(UTILS)

# Object files (mirror source structure in obj/ directory)
OBJS = $(SRCS:srcs/%.cpp=obj/%.o)

# Header files by category
CGI_HEADERS = Cgi.hpp
CONFIG_HEADERS = ConfigParser.hpp Location.hpp ServerBlock.hpp
HTTP_HEADERS = HttpRequest.hpp HttpResponse.hpp
SERVER_HEADERS = Client.hpp Router.hpp Server.hpp
UTILS_HEADERS = Logger.hpp MimeTypes.hpp utils.hpp

# add directory prefixes to header files
CGI_H = $(addprefix srcs/cgi/, $(CGI_HEADERS))
CONFIG_H = $(addprefix srcs/config/, $(CONFIG_HEADERS))
HTTP_H = $(addprefix srcs/http/, $(HTTP_HEADERS))
SERVER_H = $(addprefix srcs/server/, $(SERVER_HEADERS))
UTILS_H = $(addprefix srcs/utils/, $(UTILS_HEADERS))

# Common headers
COMMON_H = srcs/webserv.hpp

# All header file
HEADERS = $(COMMON_H) $(CGI_H) $(CONFIG_H) $(HTTP_H) $(SERVER_H) $(UTILS_H)

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
obj/%.o: srcs/%.cpp $(HEADERS)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Remove object files
clean:
	rm -rf obj/
	echo "✓ Object files removed"

# Remove object files and executable
fclean: clean
	rm -f $(NAME)
	rm -rf YoupiBanane
	rm -f test_results.json
	echo "✓ Executable removed"

# Rebuild everything from scratch
re: fclean
	$(MAKE) --no-print-directory all

# Ensure xterm is installed (WSL2 / Debian-based)
ensure-xterm:
	command -v xterm >/dev/null 2>&1 || { \
		echo "xterm not found, installing..."; \
		sudo apt-get update -qq && sudo apt-get install -y -qq xterm; \
	}

# Run the server in a new xterm with config file selection
run: $(NAME) ensure-xterm
	pkill webserv 2>/dev/null || true
	set -- config/*.conf
	echo "Select a configuration file:"
	i=1; for f; do echo "  $$i. $$f"; i=$$((i + 1)); done
	read -n1 -s choice
	shift $$((choice - 1))
	conf="$$1"
	title="$(NAME) | $$conf"
	xterm \
		-xrm 'xterm*selectToClipboard: true' \
		-xrm 'xterm*VT100.Translations: #override \n <Key>Up: ignore() \n <Key>Down: ignore() \n <Key>Left: ignore() \n <Key>Right: ignore()' \
		-fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 \
		-T "$$title" -e "bash -c 'stty -echoctl; ./$(NAME) $$conf; stty echoctl; read -p \"Press Enter to close window...\"'" &

# Kill any running instance of the local webserv binary
kill:
	bin="$$(cd "$(dir $(lastword $(MAKEFILE_LIST)))" && pwd)/$(NAME)"; \
	pids=$$(pgrep -f "$$bin" || pgrep -x "$(NAME)" || true); \
	if [ -n "$$pids" ]; then \
		kill $$pids && echo "✓ Killed running $(NAME): $$pids"; \
	else \
		echo "No running $(NAME) found"; \
	fi

test: $(NAME) ensure-xterm
	-pkill webserv || true
	xterm \
		-xrm 'xterm*selectToClipboard: true' \
		-xrm 'xterm*VT100.Translations: #override \n <Key>Up: ignore() \n <Key>Down: ignore() \n <Key>Left: ignore() \n <Key>Right: ignore()' \
		-fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 \
		-T "$(NAME) | webServTester" -e "bash -c 'stty -echoctl; ./$(NAME) config/webServTester.conf; stty echoctl; read -p \"Press Enter to close window...\"'" &
	sleep 1
	# Ensure requests is available (critical dependency)
	python3 -c 'import requests' >/dev/null 2>&1 || ( \
		echo "Installing Python package: requests"; \
		python3 -m pip install --user -q requests \
	)
	python3 webServTester.py

eval: $(NAME) ensure-xterm
	chmod +x tester cgi_tester
	mkdir -p YoupiBanane/nop
	mkdir -p YoupiBanane/Yeah
	touch YoupiBanane/youpi.bad_extension
	touch YoupiBanane/youpi.bla
	touch YoupiBanane/youpla.bla
	touch YoupiBanane/nop/youpi.bad_extension
	touch YoupiBanane/nop/other.pouic
	touch YoupiBanane/Yeah/not_happy.bad_extension
	chmod 644 YoupiBanane/youpi.bla YoupiBanane/youpla.bla
	-pkill webserv 2>/dev/null || true
	xterm \
		-xrm 'xterm*selectToClipboard: true' \
		-xrm 'xterm*VT100.Translations: #override \n <Key>Up: ignore() \n <Key>Down: ignore() \n <Key>Left: ignore() \n <Key>Right: ignore()' \
		-fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 \
		-T "$(NAME) | 42tester" -e "bash -c 'stty -echoctl; ./$(NAME) config/42tester.conf; stty echoctl; read -p \"Press Enter to close window...\"'" &
	sleep 1
	yes "" | ./tester http://localhost:8080
	echo "✓ Tests completed, stopping server..."
	-pkill webserv 2>/dev/null || true
