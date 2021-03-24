build:
	docker build spire-agent -t spire-agent
	docker-compose build

build-tests-workload:
	docker-compose exec workload ./build-tests-workload.sh

run:
	docker-compose up -d

run-server:
	docker-compose exec spire-server ./init-server.sh

create-entries:
	docker-compose exec spire-server ./create-entries.sh

generate-token:
	docker-compose exec spire-server spire-server token generate -spiffeID spiffe://example.org/host > token
	docker cp token infra_tests_1:/mnt/c-spiffe/integration_test

join-token:
	docker-compose exec $(SERVICE) spire-agent run -joinToken $(TOKEN) -config /opt/spire/conf/agent/agent.conf

clean:
	docker-compose down

integrations-tests:
	docker cp tests/run-behave-tests.sh infra_tests_1:/mnt/c-spiffe/integration_test
	docker-compose exec tests chmod +x run-behave-tests.sh
	docker-compose exec tests ./run-behave-tests.sh
	