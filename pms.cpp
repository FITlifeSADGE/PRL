#include <iostream>
#include <mpi.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <queue>

int select_min_from_queues(std::queue<int> &upper, std::queue<int> &lower, int &read_from_upper, int &read_from_lower) {
    if (upper.empty() && lower.empty()) {
        return -1;
    } else if (upper.empty()) {
        int value = lower.front();
        lower.pop();
        read_from_lower++;
        return value;
    } else if (lower.empty()) {
        int value = upper.front();
        upper.pop();
        read_from_upper++;
        return value;
    } else {
        if (upper.front() < lower.front()) {
            int value = upper.front();
            upper.pop();
            read_from_upper++;
            return value;
        } else {
            int value = lower.front();
            lower.pop();
            read_from_lower++;
            return value;
        }
    }
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int comm_rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    std::queue<int> upper;
    std::queue<int> lower;
    std::vector<int> sorted_numbers;
    // Procesor s rankem 0 načte vstupní posloupnost
    if (comm_rank == 0) {
        std::ifstream file("numbers", std::ios::binary);

        // Zkontrolujeme, zda se podařilo soubor otevřít
        if (!file.is_open()) {
            std::cerr << "Failed to open the numbers file" << std::endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        unsigned char number;
        int int_number, TAG = 1; // TAG 1 = horni vystup, TAG 2 = dolni vystup, TAG 0 = konec
        while (file.read(reinterpret_cast<char*>(&number), sizeof(number))) {
            int_number = static_cast<int>(number);
            std::cout << int_number << " ";
            MPI_Send(&int_number, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);
            TAG = TAG == 1 ? 2 : 1; // prohození TAGu
        }
        MPI_Send(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // poslání TAG 0 (konec souboru
        std::cout << std::endl;
        file.close();
    }
    else {
        while (true) {
            int number = 0;
            int to_send = 0; 
            int send_count = 0;
            int TAG = 1;
            int read_from_lower = 0;
            int read_from_upper = 0;
            MPI_Status status;
            MPI_Recv(&number, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //přijetí čísla od předchozího procesoru
            std::cout << "Process " << comm_rank << " Received " << number << " from " << status.MPI_SOURCE << " with TAG " << status.MPI_TAG << std::endl;
            if (status.MPI_TAG == 0) {
                if (comm_rank < comm_size - 1) {
                    MPI_Send(&number, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);
                }
                std::cout << "Received TAG 0, breaking " << std::endl;
                break;
            }
            if (status.MPI_TAG == 1) {
                upper.push(number);
            } else if (status.MPI_TAG == 2) {
                lower.push(number);
            }
            if ((upper.size() >= pow(2, comm_rank - 1) && lower.size() >= 1) || (lower.size() >= pow(2, comm_rank - 1) && upper.size() >= 1)) { //pokud má jedna fronta úplnou posloupnost a druhá alespoň jedno číslo
                for (int i = 0; i < (2 ^ (comm_rank - 1) + 1); i++) { // provede se 2 ^ (comm_rank - 1) + 1 přesunů
                    if (read_from_upper < (2 ^ (comm_rank - 1)) && read_from_lower < (2 ^ (comm_rank - 1))) { // Pokud ještě nebylo přečteno dost čísel z jedné fronty, vybírá se minimum z obou front
                        to_send = select_min_from_queues(upper, lower, read_from_upper, read_from_lower);
                    }
                    else if (read_from_upper == (2 ^ (comm_rank - 1))) { // pokud jsem přečetl moc čísel z horní fronty, přečtu z dolní
                        to_send = lower.front();
                        lower.pop();
                        read_from_lower++;
                        read_from_upper = 0;
                    }
                    else if (read_from_lower == (2 ^ (comm_rank - 1))) { // pokud jsem přečetl moc čísel z dolní fronty, přečtu z horní
                        to_send = upper.front();
                        upper.pop();
                        read_from_upper++;
                        read_from_lower = 0;
                    }
                    if (comm_rank < comm_size - 1) { // pokud jsem nejsem poslední procesor, pošlu číslo dalšímu procesoru
                        MPI_Send(&to_send, 1, MPI_INT, comm_rank + 1, TAG, MPI_COMM_WORLD);
                        std::cout << "Process " << comm_rank << " Sent " << to_send << " to " << comm_rank + 1 << " with TAG " << TAG << std::endl;
                        send_count++;
                        if (TAG == 1) { // prohození TAGu po odeslání 2 ^ (comm_rank) čísel
                            if (send_count == pow(2, comm_rank)) {
                                TAG = 2;
                                send_count = 0;
                            }
                        
                        }
                        else if (TAG == 2) { // prohození TAGu po odeslání 2 ^ (comm_rank) čísel
                            if (send_count == pow(2, comm_rank)) {
                                TAG = 1;
                                send_count = 0;
                            }
                        }
                    }
                }
            }
            if (comm_rank == comm_size - 1) { // pokud jsem poslední procesor, přidám číslo do vektoru
                sorted_numbers.push_back(number);
            }
        }
    }
    MPI_Finalize();
    std::cout << "Finished" << std::endl;
    if (comm_rank == comm_size - 1) {
        std::cout << "Processor " << comm_rank << ": Sorted numbers: ";
        for (int i = 0; i < sorted_numbers.size(); i++) {
            std::cout << sorted_numbers[i] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}

