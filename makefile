all:
	rm -f chudnovsky.o pi
	g++ -std=c++17 Utils.cpp -c -o utils.o
	g++ -std=c++17 chudnovsky.cpp -c -o chudnovsky.o
	g++ -std=c++17 main.cpp chudnovsky.o utils.o -o pi -lgmpxx -lgmp -lpthread -lboost_thread
performance: optim
	./pi -p 100000000 -m -w 2 -n
	./pi -p 100000000 -m -w 4 -n
	./pi -p 100000000 -m -w 8 -n
optim:
	rm -f chudnovsky.o pi
	g++ -std=c++17 Utils.cpp -c -O3 -o utils.o
	g++ -std=c++17 chudnovsky.cpp -c -O3 -o chudnovsky.o
	g++ -std=c++17 main.cpp chudnovsky.o utils.o -O3 -o pi -lgmpxx -lgmp -lpthread -lboost_thread
valgrind:
	valgrind  --leak-check=full --show-leak-kinds=all ./pi -p 1000000 -m -n
perfstat:
	perf stat -B ./pi -p 100000000 -m -n
hotspot: debug
	perf record -g -F 100 --call-graph dwarf ./pi -p 10000000 -m -n
	hotspot perf.data
test: debug
	rm -f verifier
	g++ -std=c++17 verifier.cpp -o verifier
	rm -f test_result.txt
	./pi -p 10000 -sm -v 1; diff pi_concurrent.txt pi_normal.txt | wc -l >> test_result.txt
	./pi -p 10000 -m -v 2; diff pi_concurrent.txt pi_normal.txt | wc -l >> test_result.txt
	./pi -p 10000 -m -v 3; diff pi_concurrent.txt pi_normal.txt | wc -l >> test_result.txt
	./pi -p 10000000 -sm -v 3; diff pi_concurrent.txt pi_normal.txt | wc -l >> test_result.txt
	cat test_result.txt
	./verifier
debug:
	rm -f chudnovsky.o pi
	g++ -std=c++17 Utils.cpp -c -g -o utils.o
	g++ -std=c++17 chudnovsky.cpp -c -g -o chudnovsky.o
	g++ -std=c++17 main.cpp chudnovsky.o utils.o -g -o pi -lgmpxx -lgmp -lpthread -lboost_thread
origin:
	rm -f ori
	g++ -std=c++17 chudnovsky.origin.cpp -o ori -lgmpxx -lgmp
