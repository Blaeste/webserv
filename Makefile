# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 10:08:04 by eschwart          #+#    #+#              #
#    Updated: 2026/02/08 18:55:46 by gdosch           ###   ########.fr        #
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
.ONESHELL:
.PHONY: all clean fclean re kill

# Executable name
NAME = webserv

# Compiler and flags
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -O3 -std=c++98
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

# Kill any running instance of the local webserv binary
kill:
	@bin="$$(cd "$(dir $(lastword $(MAKEFILE_LIST)))" && pwd)/$(NAME)"; \
	pids=$$(pgrep -f "$$bin" || pgrep -x "$(NAME)" || true); \
	if [ -n "$$pids" ]; then \
		kill $$pids && echo "✓ Killed running $(NAME): $$pids"; \
	else \
		echo "No running $(NAME) found"; \
	fi

test: re
	-pkill webserv || true
	gnome-terminal --geometry=160x50 -- bash -c './webserv config/default.conf; exec bash' &
	sleep 1
	@# Ensure the Python dependency "requests" is available for the tester
	@python3 -c 'import requests' >/dev/null 2>&1 || ( \
		echo "Installing missing Python package: requests"; \
		python3 -m pip --version >/dev/null 2>&1 || python3 -m ensurepip --upgrade >/dev/null 2>&1; \
		python3 -m pip install --user -q requests \
	)
	@# Ensure net-tools (netstat) is available for the tester
	@command -v netstat >/dev/null 2>&1 || ( \
		echo "Installing missing package: net-tools"; \
		sudo apt update && sudo apt install -y net-tools \
	)
	@# Ensure siege is available for the tester
	@command -v siege >/dev/null 2>&1 || ( \
		echo "Installing missing package: siege"; \
		sudo apt update && sudo apt install -y siege \
	)
	@# Ensure php-cgi is available for the tester
	@command -v php-cgi >/dev/null 2>&1 || ( \
		echo "Installing missing package: php-cgi"; \
		sudo apt update && sudo apt install -y php-cgi \
	)
	python3 webServTester.py

eval: re
	@test -f tester || wget -q https://cdn.intra.42.fr/document/document/44506/tester
	@test -f cgi_tester || wget -q https://cdn.intra.42.fr/document/document/44507/cgi_tester
	@chmod +x tester cgi_tester
	@cat > config/eval.conf <<-'EOF'
	server {
	    listen 8080;
	    server_name localhost;
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
	gnome-terminal --geometry=160x50 -- bash -c './webserv config/eval.conf; exec bash' &
	@sleep 1
	./tester http://localhost:8080
	@echo "✓ Tests completed, stopping server..."
	@-pkill webserv 2>/dev/null || true

clear_eval:
	@rm -rf YoupiBanane
	@rm -f tester cgi_tester
	@rm -f config/eval.conf
