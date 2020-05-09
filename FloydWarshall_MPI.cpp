#include <iostream>
#include <stdlib.h>
#include <ctime>
#include "mpi.h"

#define INF 12

using namespace std;


//  Вывод матрицы на экран
void printMatrix(int* matr, int vert){

    for (size_t i = 0; i < vert; i++)
    {
        for (size_t j = 0; j < vert; j++)
        {
            cout.setf(ios::left);
            if(matr[i * vert + j] >= INF)
                cout << "inf ";
            else
                cout << matr[i * vert + j] <<" ";
        }
        cout << endl;
    }
}


void intputMatr(int* matr, int vert){
    int tmp;
    for (size_t i = 0; i < vert; i++)
    {
        for (size_t j = 0; j < vert; j++)
        {
            cin >> tmp;
            if(tmp == 0)
                matr[i * vert + j] = 0;
            else if (tmp == -1) {
               matr[i * vert + j] = INF;
            }
            else matr[i * vert + j] = tmp;
        }
    }
}

void intputMatrRand(int* matrix, int vert){
    
    srand(time(NULL));
    int random;
    for (long i = 0; i < vert; i++){
		for (long j = 0; j < vert; j++) {
			if (i == j)
				matrix[i * vert + j] = 0;
			else 
			{
				random = rand() % INF;
				if (random == 0)
				{
					matrix[i * vert + j] = INF;
					matrix[j * vert + i] = INF;
				}
				else
				{
					matrix[i * vert + j] = random;
					matrix[j * vert + i] = random;
				}
			}
		}
    }
}

int main(int argc, char *argv[])
{   

    int procRank, procNumber;
    double timeStart, timeFinish;
   

    MPI_Init(&argc, &argv);	// Инициализация MPI
	MPI_Comm_size(MPI_COMM_WORLD, &procNumber); // Получение числа процессов	
	MPI_Comm_rank(MPI_COMM_WORLD, &procRank); // Получение рангов процессов


    int vert = 0;//количество вершин графа(размер матрицы)
  

    if (procRank == 0)
    {
        cout << "Enter number of vertices: ";
        cin >> vert;
    }
    
    int *matr = NULL;
    int *kRow = new int[vert];//k - строка для каждого процесса

    int rowsCount = vert / procNumber;  // кол-во передаваемых строк каждому узлу, 
                                        //которые кратны кол-ву узлов

    int remaningRows = vert  % procNumber; /* остаточное количество строк, 
                                            которые будут распределены между узлами*/

    MPI_Status status;
    MPI_Request request;

    int *strCount = new int[procNumber]; // Количество передоваемых строк каждому процессу
	int *displacement = new int[procNumber]; // Смещение
	int *numCount = new int[procNumber];//// Количество передоваемых элементов
    
    int choose = 0;

    if (procRank == 0)
    {
        matr = new int[vert * vert];
        cout << "Choose mode:\n 1: Test Mode\n 2: Evaluation Mode\nYour choice: ";
        while (choose != 1 && choose != 2)
		    std::cin >> choose;

        switch (choose)
        {
        case 1:
            intputMatr(matr,vert);
            break;
        case 2:
            intputMatrRand(matr,vert);
            break;
        }

        for (size_t i = 0; i < procNumber; i++)
        {
            strCount[i] = rowsCount;
        }

        for(size_t i = 0; remaningRows != 0;  remaningRows--, i++){
            strCount[i]++;
            if(i == procNumber - 1){
                i = 0;
            }
        }

        for (size_t i = 0; i < procNumber; i++)
        {
            numCount[i] = vert * strCount[i];
        }

        displacement[0] = 0;
        for(size_t i = 1; i < procNumber; i++){
			displacement[i] = displacement[i - 1] + numCount[i - 1];
		}

    }

    MPI_Bcast(strCount, procNumber, MPI_INT, 0, MPI_COMM_WORLD);
	MPI_Bcast(numCount, procNumber, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(displacement, procNumber, MPI_INT, 0, MPI_COMM_WORLD);

    int *matr2 = new int[numCount[procRank]];
    timeStart = MPI_Wtime();
    
    //рассылаем всем узлам матрицу
    MPI_Scatterv(matr, numCount, displacement, MPI_INT, matr2, numCount[procRank], MPI_INT, 0, MPI_COMM_WORLD);

    for (size_t k = 0; k < vert; k++)
    {   
        if(procRank == 0){

            for (size_t i = 1; i < procNumber; i++)
            {
            	//отправим всем узлам К стороку
                MPI_Isend(matr + k * vert, vert, MPI_INT, i, 0, MPI_COMM_WORLD, &request);
            }

            for (int i = 0; i < strCount[procRank]; i++){
				for (int j = 0; j < vert; j++){
                    matr2[i * vert + j] = std::min(matr2[i * vert + j], 
                                                    matr2[i * vert + k] + matr[k * vert + j]);
                }
			}

        } else {
         
            MPI_Recv(kRow, vert, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            for (int i = 0; i < strCount[procRank]; i++){
                for (int j = 0; j < vert; j++){
                    matr2[i * vert + j] = std::min(matr2[i * vert + j], 
                                                    matr2[i * vert + k] + kRow[j]);
                }
            }
        }
    }
    MPI_Gatherv(matr2, numCount[procRank], MPI_INT, matr, numCount, displacement,MPI_INT, 0, MPI_COMM_WORLD);
    
    if (procRank == 0) {
        double endTime = MPI_Wtime();
        if (choose == 1) {
            cout << "Matr: " << endl;
		    printMatrix(matr, vert);
        }
        else cout << "time: " << endTime - timeStart;
    }

    delete kRow;
    delete numCount;
    delete strCount;
    delete displacement;
    delete matr;

    return 0;
}
