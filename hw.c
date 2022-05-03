#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <time.h>

#define MAXLINE 80
#define MAXARGS 80

struct Memory {
    int address, data, occupied;
};

struct PageTable {
    int v_page_num, valid_bit, dirty_bit, page_num, time_stamp;
};

struct Memory main_memory[32];
struct Memory virtual_memory[128];
struct PageTable p_table[16];
int fifo = 0, lru = 0;
int address_order[4];
int address_counter = 0;
int lru_order[4];
int lru_counter;

int get_page_num(int virtual_addr) {
    int v_page = virtual_addr/8;
    int page = 0;

    int i;
    for (i = 0; i < 16; i++) {
        if (i == v_page) {
            page = p_table[i].page_num;
            break;
        }
    }

    return page;
}

void copy_to_disk(int page_to_evict) {
    int main_starting_index = page_to_evict * 8;
    int starting_address = main_memory[main_starting_index].address;
    int ending_address = starting_address + 8;

    for (starting_address; starting_address < ending_address; starting_address++) {
        virtual_memory[starting_address].address = main_memory[main_starting_index].address;
        virtual_memory[starting_address].data = main_memory[main_starting_index].data;
        main_starting_index++;
    }
}

void shift_arr_fifo() {
    int i;
    for (i = 0; i < 3; i++) {
        address_order[i] = address_order[i + 1];
    }
}

void move_lru(int page_num) {
    int index;

    int i;
    for (i = 0; i < 4; i++) {
        if (lru_order[i] == page_num) {
            index = i;
        }
    }

    int temp = page_num;

    int p;
    for (p = index; index < 3; index++) {
        lru_order[index] = lru_order[index + 1];
    }

    lru_order[3] = temp;
}

void execute(char* args[MAXARGS]) {
    if (!strcmp(args[0], "read")) {
        int virtual_addr = atoi(args[1]);
        int offset = virtual_addr % 8;
        int virtual_page = virtual_addr / 8;
        int page_num = 0;
        int main_memory_counter = 0; 
        
        if (p_table[virtual_page].valid_bit == 0) {
            printf("\nA Page Fault Has Occurred\n");
            
            if (address_counter == 4) {
                int page_to_evict;
                int evicted_table_num;
                int shift_index = 0;

                if (fifo) {
                    page_to_evict = address_order[0];
                } else {
                    page_to_evict = lru_order[0];
                }
                
                int main_starting_index = page_to_evict * 8;
                int virtual_starting_index = virtual_addr - (virtual_addr % 8);
                int end = main_starting_index + 8;

                int old_virtual_page = main_memory[main_starting_index].address / 8;
                p_table[old_virtual_page].valid_bit = 0;
                p_table[old_virtual_page].page_num = p_table[old_virtual_page].v_page_num;

                for (main_starting_index; main_starting_index < end; main_starting_index++) {
                    main_memory[main_starting_index].address = virtual_memory[virtual_starting_index].address;
                    main_memory[main_starting_index].data = virtual_memory[virtual_starting_index].data;
                    main_memory[main_starting_index].occupied = 1;
                    virtual_starting_index++;
                }

                page_num = (main_starting_index / 8) - 1;
                p_table[virtual_page].valid_bit = 1;
                p_table[virtual_page].page_num = page_num;

                if (fifo) {
                    shift_arr_fifo();
                    address_order[3] = page_num;  
                } else {
                    lru_order[0] = page_num;
                    move_lru(page_num);
                }
                
                int physical_addr = page_num * 8 + offset;
                printf("\n%i\n", main_memory[physical_addr].data);
            } else {
                while (main_memory[main_memory_counter].occupied != 0) {
                    main_memory_counter++;
                } 

                int virtual_memory_counter = virtual_addr - (virtual_addr % 8);
                int main_memory_end = main_memory_counter + 8;

                for (main_memory_counter; main_memory_counter < main_memory_end; main_memory_counter++) {
                    main_memory[main_memory_counter].address = virtual_memory[virtual_memory_counter].address;
                    main_memory[main_memory_counter].data = virtual_memory[virtual_memory_counter].data;
                    main_memory[main_memory_counter].occupied = 1;
                    virtual_memory_counter++;
                }

                page_num = (main_memory_counter / 8) - 1;
                p_table[virtual_page].valid_bit = 1;
                p_table[virtual_page].page_num = page_num;

                address_order[address_counter] = page_num;
                address_counter++;

                lru_order[lru_counter] = page_num;
                lru_counter++;
                
                int physical_addr = page_num * 8 + offset;
                printf("\n%i\n", main_memory[physical_addr].data);
            }
        } else {
            page_num = get_page_num(virtual_addr);

            if (lru) {
                move_lru(page_num);
            }

            int physical_addr = page_num * 8 + offset;
            printf("\n%i\n", main_memory[physical_addr].data);
        }
    } else if (!strcmp(args[0], "write")) {
        int virtual_addr = atoi(args[1]);
        int data_to_add = atoi(args[2]);
        int offset = virtual_addr % 8;
        int virtual_page = virtual_addr / 8;
        int page_num = 0;
        int main_memory_counter = 0; 
        
        if (p_table[virtual_page].valid_bit == 0) {
            printf("\nA Page Fault Has Occurred\n");

            if (address_counter == 4) {
                int page_to_evict;
                int shift_index = 0;
                int evicted_table_num;

                if (fifo) {
                    page_to_evict = address_order[0];
                } else {
                    page_to_evict = lru_order[0];
                }

                copy_to_disk(page_to_evict);

                int main_starting_index = page_to_evict * 8;
                int virtual_starting_index = virtual_addr - (virtual_addr % 8);
                int end = main_starting_index + 8;

                int old_virtual_page = main_memory[main_starting_index].address / 8;
                p_table[old_virtual_page].valid_bit = 0;
                p_table[old_virtual_page].time_stamp = 0;
                p_table[old_virtual_page].dirty_bit = 0;
                p_table[old_virtual_page].page_num = p_table[old_virtual_page].v_page_num;

                for (main_starting_index; main_starting_index < end; main_starting_index++) {
                    main_memory[main_starting_index].address = virtual_memory[virtual_starting_index].address;
                    main_memory[main_starting_index].data = virtual_memory[virtual_starting_index].data;
                    main_memory[main_starting_index].occupied = 1;
                    virtual_starting_index++;
                }

                page_num = (main_starting_index / 8) - 1;
                p_table[virtual_page].valid_bit = 1;
                p_table[virtual_page].page_num = page_num;
                p_table[virtual_page].dirty_bit = 1;
                
                if (fifo) {
                    shift_arr_fifo();
                    address_order[3] = page_num;  
                } else {
                    lru_order[0] = page_num;
                    move_lru(page_num);
                }

                int physical_addr = page_num * 8 + offset;
                main_memory[physical_addr].data = data_to_add;
            } else {
                while (main_memory[main_memory_counter].occupied != 0) {
                    main_memory_counter++;
                } 
                
                int virtual_memory_counter = virtual_addr - (virtual_addr % 8);
                int main_memory_end = main_memory_counter + 8;

                for (main_memory_counter; main_memory_counter < main_memory_end; main_memory_counter++) {
                    main_memory[main_memory_counter].address = virtual_memory[virtual_memory_counter].address;
                    main_memory[main_memory_counter].data = virtual_memory[virtual_memory_counter].data;
                    main_memory[main_memory_counter].occupied = 1;
                    virtual_memory_counter++;
                }

                page_num = (main_memory_counter / 8) - 1;
                p_table[virtual_page].valid_bit = 1;
                p_table[virtual_page].page_num = page_num;
                p_table[virtual_page].dirty_bit = 1;

                address_order[address_counter] = page_num;
                address_counter++;

                lru_order[lru_counter] = page_num;
                lru_counter++;

                int physical_addr = page_num * 8 + offset;
                main_memory[physical_addr].data = data_to_add;
            }
        } else {
            page_num = get_page_num(virtual_addr);
            p_table[virtual_page].dirty_bit = 1;

            if (lru) {
                move_lru(page_num);
            }

            int physical_addr = page_num * 8 + offset;
            main_memory[physical_addr].data = data_to_add;
        }
    } else if (!strcmp(args[0], "showmain")) {
        int index;
        int ppn = atoi(args[1]); 

        switch(ppn) {
            case 0:
                index = 0;
                break;
            case 1:
                index = 8;
                break;
            case 2:
                index = 16;
                break;
            case 3:
                index = 24;
                break;
        }

        int i;
        for (i = index; i < index + 8; i++) {
            printf("%i: %i\n", i, main_memory[i].data);
        }
    } else if (!strcmp(args[0], "showptable")) {
        int i;
        for (i = 0; i < 16; i++) {
            printf("%i:%i:%i:%i\n", i, p_table[i].valid_bit, p_table[i].dirty_bit, p_table[i].page_num);
        }
    }
}

void tokenize(char* input, char* args[MAXARGS]) {
    int i = 0;
    *args = strtok(input, " \t\n");
    
    while (1) {
        if (args[i] == NULL) {
            break;
        }

        args[++i] = strtok(NULL, " \t\n");
    }
}

void init() {
    int i;
    for (i = 0; i < sizeof(main_memory)/sizeof(main_memory[0]); i++) {
        main_memory[i].data = -1;
        main_memory[i].address = i;
        main_memory[i].occupied = 0;
    }

    for (i = 0; i < sizeof(virtual_memory)/sizeof(virtual_memory[0]); i++) {
        virtual_memory[i].data = -1;
        virtual_memory[i].address = i;
    }

    for (i = 0; i < sizeof(p_table)/sizeof(p_table[0]); i++) {
        p_table[i].v_page_num = p_table[i].page_num = i;
        p_table[i].valid_bit = p_table[i].dirty_bit = 0;
        p_table[i].time_stamp = 0;
    }
}

int main(int argc, char** argv) {
    if (argv[1] == NULL || strcmp(argv[1], "FIFO") == 0) {
        fifo = 1;
    } else if (strcmp(argv[1], "LRU") == 0) {
        lru = 1;
    }
        
    init();
    
    char input[100];
    char* args[MAXARGS];

    do {
        printf("\n> ");
        fgets(input, MAXLINE, stdin);
        tokenize(input, args);

        if (strcmp(args[0], "quit") == 0) {
            exit(0);
        }
            
        execute(args);
    } while (1);

    return 0;
}
