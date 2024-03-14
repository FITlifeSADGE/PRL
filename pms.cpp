/* Autor: Lukáš Kaprál (xkapra00)
*  Datum: 2024-04-03
*  Projekt: Pipeline Merge Sort
*/
#include <iostream>
#include <mpi.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <queue>

int select_max_from_queues(std::queue<int> &upper, std::queue<int> &lower, int *to_send_from_lower, int *to_send_from_upper) {
    if (upper.empty() && lower.empty()) {
        return -1;
    }
    // Zvolíme z horní, pokud není prázdná, můžeme z ní posílat a dolní je prázdná nebo má menší než horní nebo nemůžeme posílat z dolní
    bool selectUpper = (!upper.empty() && (*to_send_from_upper > 0 || lower.empty()) && (lower.empty() || upper.front() > lower.front() || *to_send_from_lower <= 0));

    // Zvolíme z dolní, pokud není prázdná, můžeme z ní posílat a horní je prázdná nebo má menší než dolní nebo nemůžeme posílat z horní
    bool selectLower = (!lower.empty() && (*to_send_from_lower > 0 || upper.empty()) && (upper.empty() || lower.front() >= upper.front() || *to_send_from_upper <= 0));

    if (selectUpper) {
        int value = upper.front();
        upper.pop();
        *to_send_from_upper -= 1;
        return value;
    } else if (selectLower) {
        int value = lower.front();
        lower.pop();
        *to_send_from_lower -= 1;
        return value;
    }

    // Kdyby se něco pokazilo
    return -1;
}

void TAG_swap (int *TAG, int *send_count, int *to_send_from_lower, int *to_send_from_upper, int comm_rank) {
    // Pokud procesor odeslal 2^(rank) hodnot, prohodí výstup
    if (*send_count == pow(2, comm_rank)) {
        *send_count = 0; // Opět budeme počítat počet odeslaných od 0
        *to_send_from_lower = pow(2, comm_rank - 1); // Bude zbývat odeslat 2^(rank-1) hodnot z dolní fronty
        *to_send_from_upper = pow(2, comm_rank - 1); // Bude zbývat odeslat 2^(rank-1) hodnot z horní fronty
        if (*TAG == 1 || *TAG == 2) { // Kdyby přišel TAG 0, nestane se nic
            *TAG = *TAG == 1 ? 2 : 1; // Prohodí TAG
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
            MPI_Ssend(&int_number, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);
            TAG = TAG == 1 ? 2 : 1; // prohození TAGu
        }
        MPI_Ssend(&number, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // poslání TAG 0 (konec souboru
        std::cout << std::endl;
        file.close();
    }
    else {
        int number = 0;
        int to_send = 0; 
        int send_count = 0;
        int TAG = 1;
        int to_send_from_upper = pow(2, comm_rank - 1);
        int to_send_from_lower = pow(2, comm_rank - 1);
        MPI_Status status;
        while (true) {        
            MPI_Recv(&number, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); //přijetí čísla od předchozího procesoru
            if (status.MPI_TAG == 0) {
                if (comm_rank < comm_size - 1) {
                    int rest = 1;
                    while (true) {
                        rest = select_max_from_queues(upper, lower, &to_send_from_lower, &to_send_from_upper);
                        if (rest == -1) {
                            break;
                        }
                        MPI_Ssend(&rest, 1, MPI_INT, comm_rank + 1, TAG, MPI_COMM_WORLD);
                        send_count++;
                        TAG_swap(&TAG, &send_count, &to_send_from_lower, &to_send_from_upper, comm_rank);
                    }
                    MPI_Ssend(&rest, 1, MPI_INT, comm_rank + 1, 0, MPI_COMM_WORLD);
                }
                else {
                    int rest = 1;
                    while (true) {
                        rest = select_max_from_queues(upper, lower, &to_send_from_lower, &to_send_from_upper);
                        if (rest == -1) {
                            break;
                        }
                        sorted_numbers.push_back(rest);
                    }
                }
                //std::cout << "Received TAG 0, breaking " << std::endl;
                break;
            }
            if (status.MPI_TAG == 1) {
                upper.push(number);
            } else if (status.MPI_TAG == 2) {
                lower.push(number);
            }
            if ((upper.size() >= pow(2, comm_rank - 1) && lower.size() >= 1)) { //pokud má jedna fronta úplnou posloupnost a druhá alespoň jedno číslo
                to_send = select_max_from_queues(upper, lower, &to_send_from_lower, &to_send_from_upper);
                if (comm_rank < comm_size - 1) { // pokud jsem nejsem poslední procesor, pošlu číslo dalšímu procesoru
                    MPI_Ssend(&to_send, 1, MPI_INT, comm_rank + 1, TAG, MPI_COMM_WORLD);
                    send_count++;
                    TAG_swap(&TAG, &send_count, &to_send_from_lower, &to_send_from_upper, comm_rank);
                }
                else { // pokud jsem poslední procesor, přidám číslo do vektoru
                    sorted_numbers.push_back(to_send); //
                }
            }
            
        }
    }
    MPI_Finalize();
    //std::cout << "Finished" << std::endl;
    if (comm_rank == comm_size - 1) {
        std::cout << "Processor " << comm_rank << ": Sorted numbers: ";
        for (int i = 0; i < sorted_numbers.size(); i++) {
            std::cout << sorted_numbers[i] << " ";
        }
        std::cout << std::endl;
    }
    return 0;
}

