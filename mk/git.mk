# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    git.mk                                             :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/23 10:05:37 by eschwart          #+#    #+#              #
#    Updated: 2025/12/23 10:07:53 by eschwart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# ================================
# GIT WORKFLOW
# ================================

branch-%:
	$(MAKE) branch BRANCH=$*

branch:
	@if [ -z "${BRANCH}" ]; then \
		${call say, ✖ Wrong branch name use "make branch-branchname", ${RED}}; \
		exit 1; \
	fi
	@if git show-ref --verify --quiet refs/heads/${BRANCH}; then \
		${call say, ⚠ Branch « ${BRANCH} » already exist localy., ${YELLOW}}; \
		exit 1; \
	else \
		${call say, ➕ Create branch « $(BRANCH) », ${GREEN}}; \
		git switch -c ${BRANCH}; \
	fi
	@${call say, ⬆️  Push & upstream on « origin », ${CYAN}}
	@git push origin -u ${BRANCH}

check:
	@git add .
	@git status
	@printf '$(CYAN)👉 Go for that ? [Y/N] $(RESET)'
	@read ans; \
	if [ "$$ans" = "y" ] || [ "$$ans" = "Y" ]; then \
		printf '%b' '$(GREEN) ✏  Commit message: $(RESET)'; \
		read msg; \
		git commit -m "$$msg"; \
		${call say, ✅ Commit done!, ${GREEN}}; \
		git push; \
		${call say, ⬆️  Pushed!, ${GREEN}}; \
	else \
		${call say, ℹ️  Aborded!, ${RED}}; \
	fi

reset:
	@current=$$(git rev-parse --abbrev-ref HEAD); \
	${call say, ⏪  going back on main and update..., ${CYAN}}; \
	git checkout main; \
	git reset --hard origin/main; \
	git pull --ff-only origin main; \
	if [ "$$current" != "main" ]; then \
		$(call say, 🗑  Deleting old local branch…, $(COLOR_FG_CYAN)); \
		git branch -D "$$current"; \
	else \
		$(call say, ℹ️  Already on « main » : no local branch to suppress, $(YELLOW)); \
	fi; \
	$(call say, ✅  Reset done., $(GREEN))

.PHONY: branch-% branch check reset
