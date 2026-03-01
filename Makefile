# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 10:08:04 by eschwart          #+#    #+#              #
#    Updated: 2026/03/01 20:12:16 by gdosch           ###   ########.fr        #
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
.PHONY: all clean fclean re kill test eval clear_eval

# Executable name
NAME = webserv

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -O3 -std=c++98

# Compile multi-cpu (linux only comment it on other system)
MAKEFLAGS += -j$(shell nproc)

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

# Header files by category
CGI_HEADERS = CGI.hpp
CONFIG_HEADERS = Config.hpp Location.hpp ServerConfig.hpp
HTTP_HEADERS = HttpRequest.hpp HttpResponse.hpp
SERVER_HEADERS = Client.hpp Router.hpp Server.hpp
UTILS_HEADERS = Logger.hpp MimeTypes.hpp utils.hpp

# add directory prefixes to header files
CGI_H = $(addprefix srcs/cgi/, $(CGI_HEADERS))
CONFIG_H = $(addprefix srcs/config/, $(CONFIG_HEADERS))
HTTP_H = $(addprefix srcs/http/, $(HTTP_HEADERS))
SERVER_H = $(addprefix srcs/server/, $(SERVER_HEADERS))
UTILS_H = $(addprefix srcs/utils/, $(UTILS_HEADERS))

# All header file
HEADERS = $(CGI_H) $(CONFIG_H) $(HTTP_H) $(SERVER_H) $(UTILS_H)

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
	echo "✓ Executable removed"

# Rebuild everything from scratch
re: fclean all

# Kill any running instance of the local webserv binary
kill:
	@bin="$$(cd "$(dir $(lastword $(MAKEFILE_LIST)))" && pwd)/$(NAME)"; \
	pids=$$(pgrep -f "$$bin" || pgrep -x "$(NAME)" || true); \
	if [ -n "$$pids" ]; then \
		kill $$pids && echo "✓ Killed running $(NAME): $$pids"; \
	else \
		echo "No running $(NAME) found"; \
	fi

test: $(NAME)
	-pkill webserv || true
	@xterm -xrm 'xterm*selectToClipboard: true' -fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 -T "$(NAME) | webServTester" -e "bash -c 'stty -echoctl; ./$(NAME) config/webServTester.conf; stty echoctl; read -p \"Press Enter to close window...\"'" &
	sleep 1
	@# Ensure requests is available (critical dependency)
	@python3 -c 'import requests' >/dev/null 2>&1 || ( \
		echo "Installing Python package: requests"; \
		python3 -m pip install --user -q requests \
	)
	python3 webServTester.py

eval: $(NAME)
	@test -f tester || wget -q https://cdn.intra.42.fr/document/document/44506/tester
	@test -f cgi_tester || wget -q https://cdn.intra.42.fr/document/document/44507/cgi_tester
	@chmod +x tester cgi_tester
	@cat > config/eval.conf <<-'EOF'
	server {
	    listen 8080;
	    server_name 42tester;
	    error_page 400 /error_pages/400.html;
	    error_page 403 /error_pages/403.html;
	    error_page 404 /error_pages/404.html;
	    error_page 405 /error_pages/405.html;
	    error_page 413 /error_pages/413.html;
	    error_page 500 /error_pages/500.html;
	    error_page 501 /error_pages/501.html;
	    error_page 504 /error_pages/504.html;
		client_max_body_size 105M;

	    # / - GET requests ONLY
	    location / {
	        root ./www;
	        index index.html index.htm;
	        allowed_methods GET;
	        autoindex on;
	    }

		# /post_body - POST requests with maxBody of 100 bytes
		location /post_body {
		    root ./www;
		    allowed_methods POST;
		    client_max_body_size 100;
		}

		# /directory/
		location /directory {
		    root ./YoupiBanane;
		    index youpi.bad_extension;
		    allowed_methods GET POST;
		    autoindex off;
		    cgi_extension .bla;
		    cgi_path ./cgi_tester;
			client_max_body_size 105M;
		}
	}
	EOF

	@echo "✓ Created config/eval.conf"
	@mkdir -p YoupiBanane/nop
	@mkdir -p YoupiBanane/Yeah
	@touch YoupiBanane/youpi.bad_extension
	@touch YoupiBanane/youpi.bla
	@touch YoupiBanane/youpla.bla
	@touch YoupiBanane/nop/youpi.bad_extension
	@touch YoupiBanane/nop/other.pouic
	@touch YoupiBanane/Yeah/not_happy.bad_extension
	@chmod 644 YoupiBanane/youpi.bla YoupiBanane/youpla.bla
	@-pkill webserv 2>/dev/null || true
	@xterm -xrm 'xterm*selectToClipboard: true' -fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 -T "$(NAME) | 42tester" -e "bash -c 'stty -echoctl; ./$(NAME) config/eval.conf; stty echoctl; read -p \"Press Enter to close window...\"'" &
	@sleep 1
	yes "" | ./tester http://localhost:8080
	@echo "✓ Tests completed, stopping server..."
	@-pkill webserv 2>/dev/null || true

run: $(NAME)
	-pkill webserv || true
	@xterm -xrm 'xterm*selectToClipboard: true' -fa 'Monospace' -fs 11 -bg '#1E1E1E' -fg '#CCCCCC' -geometry 145x50 -T "$(NAME)" -e "bash -c 'stty -echoctl; ./$(NAME); stty echoctl; read -p \"Press Enter to close window...\"'" &

clear_eval:
	@rm -rf YoupiBanane
	@rm -f tester cgi_tester
	@rm -f config/eval.conf
