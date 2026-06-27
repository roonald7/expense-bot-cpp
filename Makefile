# --- CONFIGURATION ---
PROD_COMPOSE = docker-compose.prod.yml
DEV_COMPOSE  = docker-compose.yml

# --- TARGETS ---

.PHONY: help deploy stop logs dev build build-base clean db-shell

help: ## Show this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-15s\033[0m %s\n", $$1, $$2}'

build-base: ## [PROD] Build the heavy base image (vcpkg + dependencies)
	docker build -f builder.Dockerfile -t expense-bot-builder:latest .

deploy: ## [PROD] Deploy the production stack in the background
	docker-compose -f $(PROD_COMPOSE) up -d --build

stop: ## [PROD] Stop the production stack
	docker-compose -f $(PROD_COMPOSE) down

logs: ## [PROD] Stream production bot logs
	docker-compose -f $(PROD_COMPOSE) logs -f bot

dev: ## [DEV] Start the development database only
	docker-compose -f $(DEV_COMPOSE) up -d db

build: ## [PROD] Only build the production Docker image
	docker build -t expense-bot:latest .

db-shell: ## Access the PostgreSQL production database shell
	docker exec -it expense_bot_db_prod psql -U $$(grep DB_USER .env | cut -d '=' -f2) -d $$(grep DB_NAME .env | cut -d '=' -f2)

clean: ## Remove all containers and volumes (WARNING: Data will be lost)
	docker-compose -f $(PROD_COMPOSE) down -v
	docker-compose -f $(DEV_COMPOSE) down -v

ps: ## View running containers
	docker ps --format "table {{.Names}}\t{{.Status}}\t{{.Ports}}"
