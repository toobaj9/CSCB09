#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/sysinfo.h>
#include <string.h>
#include <time.h>

// structure to hold the value of the flags where 1 indicates true.
typedef struct {
    int show_memory;
    int show_cpu;
    int show_cores;
    int samples;
    int tdelay;
} flag_val;

// helper function to check if a character is a digit.
int is_digit(char c) {
    return (c >= '0' && c <= '9');
}

// calculate cpu_usage
float calculate_cpu_usage(long total_diff, long idle_diff)  {	
	return (total_diff == 0) ? 0.0 : (1.0 - ((float)idle_diff / total_diff)) * 100.0;
}
// helper function to get the idle and total time.
void parse_cpu_usage(char* line, long int* total_time, long int* idle_time) {
	char* ptr = line;
	int i = 0;
	long int value;
	*total_time = 0;  // Reset before accumulating
    *idle_time = 0;
	// Calculating total_time and idle_time
	while (*ptr != '\0' && i < 10) {
        if (is_digit(*ptr)) {
            value = strtol(ptr, &ptr, 10);
            *total_time += value;
            if (i == 3) {
                *idle_time = value;
            }
            i++;
        }
		ptr++;
    }
}
// Helper function to read cpu readings from file /proc/stat.
void get_cpu_usage(long int* total_time, long int* idle_time) {
	int error;
	char line[200];
    char* cpu_data;
	FILE* fp = fopen("/proc/stat", "r");
	if (fp == NULL) {
		perror("fopen");
		exit(1);
	}
	cpu_data = fgets(line, sizeof(line), fp);
	error = fclose(fp);
    if (error != 0) {
        fprintf(stderr, "fclose failed\n");
        exit(1);
    }
	if (!cpu_data) {
		perror("Error reading from /proc/stat");
		exit(1);
	}
	parse_cpu_usage(cpu_data, total_time, idle_time);
}
// printing x axis for graphs
void print_horizontal_axis(int samples) {
	printf("   ");
	for (int j = 0; j < samples + 1; j++) {
   		printf("-");
	}
	printf("\n");
   }

// Function to print graph structure x and y axis
void print_graph_structure_cpu(int samples) {
	printf("\n"); 
	printf("\n");
    for (int i = 10; i >= 1; i--) {
        if (i == 10)
            printf("  100%%  |");
		else {
			printf("        |");
		}
        printf("\n");
    }
	printf("   0%%");
    print_horizontal_axis(samples);
}

// Function to plot cpu uages on the graph in real time
void update_graph_cpu(float cpu_usage, int sample_index, int offset) {
	int height = (int)(cpu_usage / 10); //calculate appropriate height since each bar represents 10%
	height = (height == 10) ? 9 : height;
	printf("\x1b[%d;%df:", offset - height, 10 + sample_index);    /* move to position col,row to plot : */
	fflush(stdout);
}

float get_total_mem() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        exit(1);
    }

    long total_mem = 0;
	fscanf(fp, "%*s %ld", &total_mem);
    fclose(fp);  // Close the file after reading

    return (float)total_mem / (1024 * 1024);  // Convert KB to GB
}

// Function to get memory stats (dynamic values)
void get_free_mem(long *mem_free) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        exit(1);
    }
    char line[256];
    // Read the MemFree line for free memory
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemFree", 7) == 0) {
            sscanf(line, "%*s %ld", mem_free);  // Get free memory
            break;
        }
    }
    fclose(fp);
}
// Function to calculate memory usage
void calculate_mem_info(float mem_total, float* mem_usage) {
    long free_mem;
    get_free_mem(&free_mem);
    float temp = (float) (free_mem) / (1024 * 1024);
    *mem_usage = mem_total - temp;  // Convert to GB
}

void print_graph_structure_memory(int samples) {
	printf("\n"); 
	printf("\n");
	for (int i = 12; i >= 1; i--) {
		if (i == 12) {
			printf("\033[s");
			printf("        |");
			printf("\033[u");
			printf(" %.0f GB", get_total_mem());
		} else {
			printf("        |");
		}
		printf("\n");
	}
	printf(" 0 GB");
	print_horizontal_axis(samples);
}
void plot_memory_usage(float total_memory, float mem_usage, int sample_index) {
	int height = (int)((mem_usage / total_memory) * 12);
	height = (height == 12) ? 11 : height;
	printf("\x1b[%d;%df#", 15 - height , 10 + sample_index );    /* move to position col,row to plot : */
	fflush(stdout);
}

int get_cores() {
	long int num_cores = sysconf(_SC_NPROCESSORS_ONLN); //get the number of cores
	if (num_cores == -1) {
		perror("sysconf");
		exit(1);
	}
	return (int)num_cores;
}

float get_max_freq() {
    FILE *fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
    long int max_freq;

    if (fp == NULL) {
        perror("fopen"); //error opening the file
        exit(1);
    }
    // Use scanf to read the integer directly
    if (fscanf(fp, "%ld", &max_freq) != 1) {
        perror("Error reading from file");
        exit(1);
    }
    // Close the file after reading
    if (fclose(fp) != 0) {
        fprintf(stderr, "fclose failed\n"); //if not properly closed
        exit(1);
    }
    // Return the max frequency in GHz (divide by 1000000)
    return (float) max_freq / (1000000);
}

void cores_diagram(int num_cores) {
    int cores_per_row = 4;  // Number of cores per row by default if atleast 4 cores.
	int remaining_cores = num_cores; 

    for (int i = 0; i < num_cores; i += cores_per_row) {
		int cores_in_this_row = (remaining_cores >= cores_per_row) ? cores_per_row : remaining_cores;
        for (int j = 0; j < 3; j++) {  // Draw each row three times (one per box layer)
            for (int k = 0; k < cores_in_this_row; k++) {
                if (j == 0) printf("+ -- +");
                if (j == 1) printf("|    |");
                if (j == 2) printf("+ -- +");
                
                if (k < cores_in_this_row - 1) printf("   ");  // Space between cores
            }
            printf("\n");
        }
        printf("\n");  // adds spacing before next row
		remaining_cores -= cores_per_row;
    }
}

// display cpu and memory graphs based on the value of flags.
void display_info(flag_val *f) {
    
    float cpu_usage;
    long int total, idle;
    long int prev_total = 0;
    long int prev_idle = 0;
	
    float memory_usage;
    float total_mem = get_total_mem();
	int offset = (f->show_memory) ? 28 :13;
    // Set the row and offset based on whether memory is shown
    int cpu_row = (f->show_memory) ? 18 : 3;
    int mem_row = 3;

	if (f->show_memory) {
		print_graph_structure_memory(f->samples);
	}
    // Print graph structures
	if (f->show_cpu) {
		print_graph_structure_cpu(f->samples);
		get_cpu_usage(&prev_total, &prev_idle);
	}

    // Save current cursor position
    printf("\033[s");
    
    // Main loop for updating CPU and memory usage
    for (int i = 0; i < f->samples; i++) {
		if (f->show_memory) {
            calculate_mem_info(total_mem, &memory_usage);

            printf("\033[%d;1H\033[Kv Memory  %2.2f GB", mem_row, memory_usage);

            fflush(stdout);  // Ensure it gets printed immediately

            plot_memory_usage(total_mem, memory_usage, i); // Update Memory graph
        }
		
        if (f->show_cpu) {
            get_cpu_usage(&total, &idle);
            cpu_usage = calculate_cpu_usage(total - prev_total, idle - prev_idle);

			printf("\033[%d;1H\033[Kv CPU  %2.2f %%", cpu_row, cpu_usage);


            fflush(stdout);  // Ensure it gets printed immediately

            update_graph_cpu(cpu_usage, i, offset);
            prev_total = total;
            prev_idle = idle;
        }
        // Wait for the specified delay only once per iteration
        usleep(f->tdelay);
    }
	 printf("\033[u");  // Move back to the initial position after each loop iteration
}

void display_cores_info() {
	printf("\n");
	int num_cores = get_cores();
	float max_freq = get_max_freq();
	printf("v Number of Cores: %d @ %.2f GHz\n", num_cores, max_freq);
	cores_diagram(num_cores);
}
int is_valid_integer(char *str) {
	if (str[0] == '-') return 0; // Not Valid if negative numbers
	char *end_ptr;
	strtol(str, &end_ptr, 10);
	return *end_ptr == '\0';   //Only Valid if no extra characters remain.
}

void parse_arguments(int argc, char **argv, flag_val *f) {
    f->samples = 20; //default
    f->tdelay = 500000; //default
    f->show_memory = 0;
    f->show_cpu = 0;
    f->show_cores = 0;
    int pos_sample_given = 0; // Flag to check if positional arguments exist
	int pos_delay_given = 0;
	
    // positional arguments (must be first)
    int arg_index = 1;
    if (argc > 1 && argv[1][0] != '-') {
		if (!is_valid_integer(argv[1])) {
				fprintf(stderr, "Invalid positional argument\n");
				exit(1);
		} 
		f->samples = atoi(argv[1]);
		if (f->samples == 0) {
			fprintf(stderr, "Invalid positional argument\n");
			exit(1);
		}
		pos_sample_given = 1;
		arg_index++;
		
        if (argc > 2 && argv[2][0] != '-') {
			if (!is_valid_integer(argv[2])) {
				fprintf(stderr, "Invalid positional argument\n");
				exit(1);
			}
			f->tdelay = atoi(argv[2]);
			if (f->tdelay == 0) {
				fprintf(stderr, "Invalid positional argument\n");
				exit(1);
			}
            arg_index++;
			pos_delay_given = 1;
		}
	}
    // Parse flagged arguments
    for (int i = arg_index; i < argc; i++) {
        if (strcmp(argv[i], "--memory") == 0) {
			if (f->show_memory) {
                fprintf(stderr, "Error: Duplicate flag --memory.\n");
                exit(1);
            }
            f->show_memory = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
			if (f->show_cpu) {
                fprintf(stderr, "Error: Duplicate flag --cpu.\n");
                exit(1);
            }
            f->show_cpu = 1;
        } else if (strcmp(argv[i], "--cores") == 0) {
			if (f->show_cores) {
                fprintf(stderr, "Error: Duplicate flag --cores.\n");
                exit(1);
            }
            f->show_cores = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0) {
            if (pos_sample_given) {
                fprintf(stderr, "Error: Cannot use --samples when it's already provided.\n");
                exit(1);
            }
			pos_sample_given = 1;
            f->samples = atoi(argv[i] + 10);
			if (f->samples == 0 || !is_valid_integer(argv[i] + 10)) { // Check if no valid value specified
                fprintf(stderr, "Error: Invalid value for --samples.\n");
                exit(1);
            }
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
            if (pos_delay_given) {
                fprintf(stderr, "Error: Cannot use --tdelay when its already provided.\n");
                exit(1);
            }
			pos_delay_given = 1;
            f->tdelay = atoi(argv[i] + 9);
			if (f->tdelay == 0 || !is_valid_integer(argv[i] + 9)) { // Check if no valid value specified
				fprintf(stderr, "Error: Invalid value for --tdelay.\n");
                exit(1);
            }
        } else {
            fprintf(stderr, "Error: Unknown argument '%s'.\n", argv[i]);
            exit(1);
        }
    }
    // If no flag is provided, enable all by default
    if (!f->show_memory && !f->show_cpu && !f->show_cores) {
        f->show_memory = f->show_cpu = f->show_cores = 1;
    }
}

int main(int argc, char** argv) {
	flag_val f;
	parse_arguments(argc, argv, &f);
	
	printf("\033[H\033[J"); // Clear the screen
    // Call get_cpu_usage and print the result
	printf("Nbr of samples: %d -- every %d microSecs (%.3f secs)\n", f.samples, f.tdelay, (double)f.tdelay/1000000);
	
    if (f.show_memory || f.show_cpu) {
		display_info(&f);
	}
	if (f.show_cores) {
		display_cores_info();
	}
	fflush(stdout);
    return 0;
}