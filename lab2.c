#include <dirent.h> 
#include <stdio.h> 
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <iomanip>
#include <iostream>
#include <sstream>

int main(int argc, char** argv) {
	//setlocale(LC_ALL, "");
	if (argc == 2) {
		char* folder;
		char folder1[255];

		/* обработка входных параметров */
		if(argc == 1){
			getcwd(folder1, 255);
			folder = folder1;
			//printf("%s %s.\n", folder, folder1);
		}else{
			if(argv[1] != "-h"){
				folder = argv[1];
			}else{

				/* обработка ключа "-h" */
				printf("lab2. Linux API.\n");
			}
		}
		printf("folder '%s'\n", folder);
		
		/* структуры для хранения идентификаторов pipe */
		int pipefd[2];
		int pipeft[2];

		if (pipe(pipefd) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		if (pipe(pipeft) == -1) {
			perror("pipe");
			exit(EXIT_FAILURE);
		}


		/* создание потоков */
		pid_t cpid = fork();
		if (cpid == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		

		if (cpid != 0) { 

			/* клиентская часть */
			//printf("parent\n");
			//printf("parent folder '%s'\n", folder);
			//printf("parent len %i, \n", lFolder);
			close(pipefd[0]);

			/* передача серверу пути к папке */
			write(pipefd[1], folder, 255*sizeof(char));
			//close(pipefd[1]);
			
			sleep(3);
			
			/* получение ответа от сервера */
			int str_size;
			close(pipeft[1]);
			while(read(pipeft[0], &str_size, sizeof(int)) > 0){
				char line[str_size];
				read(pipeft[0], line, str_size);
				//printf("%s\n", line);
				std::cout <<  line << std::endl;
			}
			/*
			char line[255];
			while(read(pipefd[0], line, 255*sizeof(char)) > 0){
				printf("%s", line);
			}*/
		}else{
			/* серверная часть */
			//child
			//printf("child\n");
			int lFolder = 0;
			
			close(pipefd[1]);
			//read(pipefd[0], &lFolder, sizeof(int));
			char cFolder[255];

			/* получение от клиента пути к папке */
			read(pipefd[0], cFolder, 255*sizeof(char));

			printf("child folder %i, '%s'\n", lFolder, cFolder);
			

			/* структуры для хранения информации об элементах папки */
			DIR           *d;
			struct dirent *dir;
			struct stat stats;
			int exists;
			//d = opendir("/home/ubuntu/");			
			d = opendir((const char*)(cFolder));
			//close(pipeft[0]);
			if (d)
			{
				//printf("child d\n");
				while ((dir = readdir(d)) != NULL)
				{
					std::ostringstream strs;
					/* определение: файл или папка */					
					//printf("%i | ", dir->d_type);
					if((int)(dir->d_type) == 4){
						strs << "dir " << dir->d_type << "| ";
					}else{
						strs << "    " << dir->d_type << " | ";
					}
					/* получение информации об элементе */
					exists = stat(dir->d_name, &stats);
					if (exists < 0) {
						fprintf(stderr, "Couldn't stat %s\n", dir->d_name);
					} else {		
						/* получение размера */			
						double st_size = stats.st_size;
						int count = 0;
						
						while(st_size > 512){
							st_size /= 1024;
							count += 1;
						}
						
						strs << std::setw(8) << std::setprecision(2) << st_size;
						if(count == 0){
							strs << " B | ";
						}else{
							if(count == 1){
								strs << "KB | ";
							}else{
								if(count == 2){
									strs << "MB | ";
								}else{
									strs << "GB | ";
								}
							}
						}
					}

					/* получение даты последнего изменения */
					char date[25];
					strftime(date, 25, "%d-%m-%y %H:%M", localtime(&(stats.st_ctime)));
					strs << date << " | " << dir->d_name;
					std::string dir_info = strs.str();
					//std::cout <<" ___"<< strs.str() << " ___"<<dir_info.c_str()<< std::endl;
					//std::cout <<"! "<< dir_info;
					//write(pipefd[1], &(dir_info.length()), sizeof(int));
					

					/* отправка сообщения с информацией клиенту */
					const char* buf = dir_info.c_str();		
					int line_size = strlen(buf);			
					//strcmp(buf, dir_info.c_str());
					//std::cout << buf <<std::endl;
					close(pipeft[0]);
					write(pipeft[1], &line_size, sizeof(int));
					write(pipeft[1], buf, line_size);
				}

				closedir(d);
				//close(pipefd[1]);
				_exit(0);
			}
			
			fflush(stdout);
			_exit(0);
		}		

		return 0;
	}
	else return -1;
}
