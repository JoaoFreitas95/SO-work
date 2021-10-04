.DEFAULT_GOAL := simulator

compile_monitor:
	gcc Monitor/main.c -o Monitor/main -lpthread

start_monitor:
	./Monitor/main Monitor/configuration/config.txt

compile_simulator:
	gcc Simulator/main.c -o Simulator/main -lpthread

start_simulator:
	./Simulator/main Simulator/configuration/config.txt

clean:
	-rm -r Simulator/main
	-rm -r Monitor/main