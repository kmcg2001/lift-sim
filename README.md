# lift-sim
Simulating a lift request handling system with 3 lifts and at least 50 requests from people requesting to use a lift,
	 	demonstrating the producer-consumer problem. 


There are two implentations of the simulation, one using POSIX's Pthreads (lift_sim_threads) and one using processes created by POSIX's fork() with POSIX Semaphores (lift_sim_processes).

Each implementation aims to provide the same functionality, as they each input a list of lift requests, a buffer size, and a standardised lift request duration, and output a log showing the requests made, the handling of each request, and various statistics.


## Usage

lift_sim_threads:
1. Open the lift_sim_threads folder 
2. Include in the folder a file called "sim_input.txt" of 50-100 requests you want made, with min floor 1 and max floor 20 (example file given).
3. Compile the program,in the terminal:
``make``
4. Run the program, in the terminal:
     ``./lift_sim_A [buffer-size] [request duration]``
5. View the log data in the "sim.out.txt" file


lift_processes:
1. Open the lift_sim_processess folder
2. Include in the folder a file called "sim_input.txt" of 50-100 requests you want made, with min floor 1 and max floor 20 (example file given).
3. Compile the program, in the terminal:
      ``make``
4. Run the program, in the terminal:
      ``./lift_sim_B [buffer-size] [request duration]``
5. View the log data in the "sim.out.txt" file
  
 ## TODO
 * N/A
