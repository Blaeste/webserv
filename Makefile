# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/16 10:08:04 by eschwart          #+#    #+#              #
#    Updated: 2026/01/20 13:52:19 by gdosch           ###   ########.fr        #
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

eval: re
	@test -f tester || wget -q https://cdn.intra.42.fr/document/document/44506/tester
	@test -f cgi_tester || wget -q https://cdn.intra.42.fr/document/document/44507/cgi_tester
	@chmod +x tester cgi_tester
	@if [ ! -f config/eval.conf ]; then \
		echo "server {" > config/eval.conf; \
		echo "    listen 8080;" >> config/eval.conf; \
		echo "    server_name localhost;" >> config/eval.conf; \
		echo "    error_page 400 /error_pages/400.html;" >> config/eval.conf; \
		echo "    error_page 403 /error_pages/403.html;" >> config/eval.conf; \
		echo "    error_page 404 /error_pages/404.html;" >> config/eval.conf; \
		echo "    error_page 405 /error_pages/405.html;" >> config/eval.conf; \
		echo "    error_page 413 /error_pages/413.html;" >> config/eval.conf; \
		echo "    error_page 500 /error_pages/500.html;" >> config/eval.conf; \
		echo "    error_page 501 /error_pages/501.html;" >> config/eval.conf; \
		echo "    error_page 504 /error_pages/504.html;" >> config/eval.conf; \
		echo "" >> config/eval.conf; \
		echo "    # / - GET requests ONLY" >> config/eval.conf; \
		echo "    location / {" >> config/eval.conf; \
		echo "        root ./www;" >> config/eval.conf; \
		echo "        index index.html index.htm;" >> config/eval.conf; \
		echo "        allowed_methods GET;" >> config/eval.conf; \
		echo "        autoindex off;" >> config/eval.conf; \
		echo "    }" >> config/eval.conf; \
		echo "" >> config/eval.conf; \
		echo "    # /post_body - POST requests with maxBody of 100 bytes" >> config/eval.conf; \
		echo "    location /post_body {" >> config/eval.conf; \
		echo "        root ./www;" >> config/eval.conf; \
		echo "        allowed_methods POST;" >> config/eval.conf; \
		echo "        client_max_body_size 100;" >> config/eval.conf; \
		echo "    }" >> config/eval.conf; \
		echo "" >> config/eval.conf; \
		echo "    # /directory/ - GET requests, root = YoupiBanane, index = youpi.bad_extension" >> config/eval.conf; \
		echo "    location /directory/ {" >> config/eval.conf; \
		echo "        root ./YoupiBanane;" >> config/eval.conf; \
		echo "        index youpi.bad_extension;" >> config/eval.conf; \
		echo "        allowed_methods GET;" >> config/eval.conf; \
		echo "        autoindex off;" >> config/eval.conf; \
		echo "    }" >> config/eval.conf; \
		echo "" >> config/eval.conf; \
		echo "    # Fichiers .bla - POST requests via CGI (partout où ils se trouvent)" >> config/eval.conf; \
		echo "    location /directory/youpi.bla {" >> config/eval.conf; \
		echo "        root ./YoupiBanane;" >> config/eval.conf; \
		echo "        allowed_methods POST;" >> config/eval.conf; \
		echo "        cgi_extension .bla;" >> config/eval.conf; \
		echo "        cgi_path ./cgi_test;" >> config/eval.conf; \
		echo "    }" >> config/eval.conf; \
		echo "}" >> config/eval.conf; \
		echo "✓ Created config/eval.conf"; \
	fi
	@mkdir -p YoupiBanane/nop
	@mkdir -p YoupiBanane/Yeah
	@touch YoupiBanane/youpi.bad_extension
	@touch YoupiBanane/youpi.bla
	@touch YoupiBanane/nop/youpi.bad_extension
	@touch YoupiBanane/nop/other.pouic
	@touch YoupiBanane/Yeah/not_happy.bad_extension
	@-pkill webserv 2>/dev/null || true
	@./webserv config/eval.conf &
	@sleep 1
	./tester http://localhost:8080

clear_eval:
	@rm -rf YoupiBanane
	@rm -f tester cgi_tester
	@rm -f config/eval.conf
