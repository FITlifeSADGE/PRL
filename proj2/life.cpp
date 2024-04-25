#include <iostream>
#include <mpi.h>
#include <fstream>
#include <vector>
#include <cmath>
#include <queue>

std::string change_cell(const char &cell, std::vector<char> &neighbours) {
if (cell == '0') {
    int alive_neighbours = 0;
    for (char c : neighbours) {
        if (c == '1') {
            alive_neighbours++;
        }
    }
    if (alive_neighbours == 3) {
        return "1";
    }
    else {
        return "0";
    }

}
else {
    int alive_neighbours = 0;
    for (char c : neighbours) {
        if (c == '1') {
            alive_neighbours++;
        }
    }
    if (alive_neighbours == 2 || alive_neighbours == 3) {
        return "1";
    }
    else {
        return "0";
    }
}
}


std::string get_neighbours(const std::string &line, const std::string &neighbour1, const std::string &neighbour2) {
    std::vector<char> neighbours;
    std::string final = "";
    for (int i = 0; i < line.size(); ++i) {
        neighbours.clear();

        int neighbour_left = (i == 0) ? line.size() - 1 : i - 1; // vezmu prvek nalevo
        int neighbour_right = (i == line.size() - 1) ? 0 : i + 1; // vezmu prvek napravo

        neighbours.push_back(neighbour1[neighbour_left]);
        neighbours.push_back(neighbour1[i]);
        neighbours.push_back(neighbour1[neighbour_right]);
        neighbours.push_back(neighbour2[neighbour_left]);
        neighbours.push_back(neighbour2[i]);
        neighbours.push_back(neighbour2[neighbour_right]);
        neighbours.push_back(line[neighbour_left]);
        neighbours.push_back(line[neighbour_right]);

        final += change_cell(line[i], neighbours);
    }
    return final;
}

void send_to_others(const std::string &line, int comm_rank, int comm_size) {
    if (comm_rank == 0) {
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, comm_size - 1, 0, MPI_COMM_WORLD);
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, comm_rank + 1, 1, MPI_COMM_WORLD);      
    }
    if (comm_rank == comm_size - 1) {
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, comm_rank - 1, 0, MPI_COMM_WORLD);
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, 0, 1, MPI_COMM_WORLD);
    }
    if (comm_rank != 0 && comm_rank != comm_size - 1) {
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, comm_rank - 1, 0, MPI_COMM_WORLD);
        MPI_Send(line.c_str(), line.size(), MPI_CHAR, comm_rank + 1, 1, MPI_COMM_WORLD);
    }
}

void recv_from_others(int comm_rank, int comm_size, const std::string &line, int iterations, std::string *all_lines) {
    int line_size = line.size();
    char recv_line[line_size + 1];
    char recv_line2[line_size + 1];
    MPI_Status status;
    std::string new_line = line;

    for (int i = 0; i < iterations; i++) {
        if (comm_rank == 0) {
            MPI_Recv(recv_line, line_size, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            recv_line[line_size] = '\0';

            MPI_Recv(recv_line2, line_size, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            recv_line2[line_size] = '\0';
            new_line = get_neighbours(new_line, recv_line, recv_line2);
            if (i + 1 < iterations) {
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, comm_size - 1, 0, MPI_COMM_WORLD);
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, comm_rank + 1, 1, MPI_COMM_WORLD);
            }
        }
        if (comm_rank == comm_size - 1) {
            MPI_Recv(recv_line, line_size, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            recv_line[line_size] = '\0';

            MPI_Recv(recv_line2, line_size, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            recv_line2[line_size] = '\0';
            new_line = get_neighbours(new_line, recv_line, recv_line2);
            if (i + 1 < iterations) {
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, comm_rank - 1, 0, MPI_COMM_WORLD);
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, 0, 1, MPI_COMM_WORLD);
            }
        }
        if (comm_rank != 0 && comm_rank != comm_size - 1) {
            MPI_Recv(recv_line, line_size, MPI_CHAR, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
            recv_line[line_size] = '\0';

            MPI_Recv(recv_line2, line_size, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
            recv_line2[line_size] = '\0';
            new_line = get_neighbours(new_line, recv_line, recv_line2);
            if (i + 1 < iterations) {      
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, comm_rank - 1, 0, MPI_COMM_WORLD);
                MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, comm_rank + 1, 1, MPI_COMM_WORLD);
            }
        }
    }
    if (comm_rank == 0) {
        for (int src = 1; src < comm_size; ++src) {
            MPI_Recv(recv_line, line_size, MPI_CHAR, src, 9999-src, MPI_COMM_WORLD, &status);
            recv_line[line_size] = '\0';
            all_lines[src] = recv_line;
        }
        all_lines[0] = new_line;
    }
    if (comm_rank != 0) {
        MPI_Send(new_line.c_str(), new_line.size(), MPI_CHAR, 0, 9999-comm_rank, MPI_COMM_WORLD);
    }
}



int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);

    int comm_rank, comm_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Failed to open the grid file" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    int iterations = std::stoi(argv[2]);
    std::string line;
    // Skipnu řádky, co jsou pro jiný procesory
    for (int i = 0; i < comm_rank; ++i) {
        std::getline(file, line);
    }
    // Načtu řádku pro sebe
    if (std::getline(file, line)) {
        send_to_others(line, comm_rank, comm_size);
    } else {
        std::cerr << "Něco se pokazilo" << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    std::string *all_lines = nullptr;
    if (comm_rank == 0) {
        all_lines = new std::string[comm_size];
    }
    
    recv_from_others(comm_rank, comm_size, line, iterations, all_lines);
    if (comm_rank == 0) {
        std::cout << "0: " << line << std::endl;
        for (int i = 1; i < comm_size; ++i) {
            std::cout << i << ": " << all_lines[i] << std::endl;
        }
    }
    MPI_Finalize();
    return 0;
}