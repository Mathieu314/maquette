/*
 * deuxieme.c
 * 
 * Copyright 2014 Mathieu Moneyron <mathieu.moneyron@laposte.net>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

// compile with : gcc deuxieme.c -o deuxieme -lwiringPi

#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define X 4
#define Y 10000

int temperature = 0;
	int lumiere = 0;
	int magnetique = 0;
	int presence = 0;
	int porteOuverte = 0;
	int chauffage = 0;
	int tableau[X][Y]; // id | etat | heureArrivée | heureDépart
	
	int id = 0;

int chauffer(int temperature, int presence, int id, int tableau[X][Y])
{
	int i = 0;
	time_t maintenant;
	int compteur = 0;
	if (temperature < 15000) //maintient à une température minimale de 15 degrés
	{
		return 1;
	}
	else if (temperature < 22000)
	{
		if (presence)
		{
			return 1;
		}
		maintenant = time(NULL);
		for (i = 0; i <= id; i++)
		{
			if (((maintenant + 1800)%86400 >= tableau[2][i]) && ((maintenant + 1800)%86400 <= tableau[3][i]))
			{
				++compteur;
			}
		}
		if ((((tableau[3][id] - tableau[3][0])/86400)!=0 )&& (id >= 1))
		{
			if ((compteur/((tableau[3][id] - tableau[3][0])/86400)) > 0.5) // on vérifie qu'il a bien été présent au moins une fois sur deux
			{
				
				return 1;
			}
		}
	}
	return 0;
}

void exporter(int tableau[X][Y], int id)
{
	int i = 0;
	FILE* bdd = NULL;
	bdd = fopen("/home/pi/maison/sources/bdd.txt","w");
	for (i = 0 ; i <= id ; i++)
	{
		fprintf(bdd, "id : %d,  ", tableau[0][i]);
		fprintf(bdd, "état : %d,  ",tableau[1][i]);
		fprintf(bdd, "arrivée : %d,  ", tableau[2][i]);
		fprintf(bdd, "départ : %d\n", tableau[3][i]);
	}
	fclose(bdd);
}

#define TAILLE_MAX 70

void importer(int tableau[X][Y]) {
	FILE* fichier_import = NULL;
	fichier_import = fopen("/home/pi/maison/sources/bdd2.txt","r");
	char chaine[TAILLE_MAX] = "";
	char *nombre2 = NULL;
	char limite = ',';
	if (fichier_import != NULL)
	{
		while (fgets(chaine, TAILLE_MAX, fichier_import) != NULL)
		{
			printf("%s", chaine);
			nombre2 = strndup(chaine, 6);
			//strtok(chaine, &limite);
			printf("Id : %s\n", nombre2);
		}
		fclose(fichier_import);
	}
}

void* gestionCapteurs()
{
	time_t timestamp;
	timestamp = time(NULL);

	FILE* fichier = NULL;
	system("sudo modprobe w1-gpio");
	system("sudo modprobe w1-therm");
	while (1)
	{
		delay(100);
		if (time(NULL) >= (timestamp + 1800 )) // pour 10 jours : 864000
		{
			exporter(tableau, id);
			return 0;
		}
		// Temperature
		fichier = fopen("/sys/bus/w1/devices/28-0000057b19e4/w1_slave","r");
		fseek(fichier, 69, SEEK_SET);
		fscanf(fichier,"%d",&temperature);
		//printf("Temperature : %d\n", temperature);
		fclose(fichier);
		system("gpio mode 3 out");
		if (chauffer(temperature, presence, id, tableau))
		{
			system("gpio write 3 0");
			chauffage = 1;
			//printf("Chauffage allumé.\n");
		}
		else 
		{
			system("gpio write 3 1");
			chauffage = 0;
			//printf("Chauffage éteint.\n");
		}
		
		// Lumière
		system("cd /sys/class/gpio/ && echo 17 > export");
		fichier = fopen("/sys/class/gpio/gpio17/value","r");
		fscanf(fichier,"%d",&lumiere);
		fclose(fichier);
		//printf("Lumière : %d\n", lumiere);
		system("gpio mode 4 out");
		if (lumiere & presence) // penser à vérifier qu'il ne soit pas l'heure de dormir
		{
			system("gpio write 4 1");
			//printf("Lumière allumée.\n");
		}
		else 
		{
			system("gpio write 4 0");
			//printf("Lumière éteinte.\n");
		}
		
		// Magnétique
		system("cd /sys/class/gpio/ && echo 18 > export");
		fichier = fopen("/sys/class/gpio/gpio18/value","r");
		fscanf(fichier,"%d",&magnetique);
		fclose(fichier);
		//printf("Magnétique : %d\n", magnetique);
		if (magnetique == 1)
		{
			porteOuverte = 1;
		}
		else if (porteOuverte)
		{
			if (presence == 0)
			{
				tableau[0][id] = id;
				tableau[1][id] = 0;
				tableau[2][id] = time(NULL);
			}
			else if (presence)
			{
				tableau[3][id] = time(NULL);
				tableau[1][id] = 1;
				++id;
			}
			presence = 1 - presence;
			porteOuverte = 0;
		}
		//printf("Présence : %d\n\n", presence);
	}
}

void* shell() 
{
	int continuer = 1;
	char commande[50], exit[]="exit";
	printf("Hello Dave !\n");
	while (continuer == 1) {
		printf(">: ");
		scanf("%s", &commande);
		if (strcmp(commande, exit) == 0) {
			printf("A bientôt !\n");
			system("gpio write 4 0"); // eteindre la lumiere
			lumiere = 0;
			system("gpio write 3 1"); // eteindre le chauffage
			chauffage = 0;
			continuer = 0; // Le shell se termine lorsque "exit" est entré. Cela rend l'exécution au main
		}
		else if (strcmp(commande, "temperature") == 0) {
			printf("Temperature : %d,%d°C\n", temperature/1000, temperature%1000);
		}
		else if (strcmp(commande, "presence") == 0) {
			printf("Presence : %d\n", presence);
		}
		else if (strcmp(commande, "chauffage") == 0) {
			printf("Chauffage : %d\n", chauffage);
		}
		else if (strcmp(commande, "exporter") == 0) {
			printf("Exportation en cours ...\n");
			exporter(tableau, id);
			printf("Exportation réussie.\n");
		}
		else if (strcmp(commande, "lumiere") == 0) {
			printf("Lumière : %d\n", lumiere);
		}
		else if (strcmp(commande, "importer") == 0) {
			printf("Importation en cours ...\n");
			importer(tableau);
			printf("Importation réussie.\n");
		}
			
	}
	return NULL;
}

int main()
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t gestion, commande;
	pthread_create(&gestion, &attr, gestionCapteurs, NULL);
	pthread_create(&commande, NULL, shell, NULL);
	pthread_join(commande, NULL); // On attend la fin du shell et la fonction principale se termine
	pthread_attr_destroy(&attr);
	return 0;
}