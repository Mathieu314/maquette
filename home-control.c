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

// compile with : gcc deuxieme2.c -o home-control -lpthread -lwiringPi

/********** Importation des bibliothèques de fonctions **********/
/**/   #include <stdio.h>                                     /**/
/**/   #include <stdlib.h>                                    /**/
/**/   #include <wiringPi.h>                                  /**/
/**/   #include <time.h>                                      /**/
/**/   #include <pthread.h>                                   /**/
/**/   #include <string.h>                                    /**/
/****************************************************************/


/******************* Définition des variables *******************/
/**/   #define X 3        /* Nombre de colones d'un tableau     */
/**/   #define Y 10000    /* Nombre de lignes du tableau        */
/**/   #define MAX 21000  /* Température maximale en présence   */
/**/                      /* de l'habitant                      */
/**/   #define MIN 19000  /* Température minimale en présence   */
/**/                      /* de l'habitant                      */
/**/   #define MIN_LOW 15000 /* Température minimale en absence */
/**/                         /* de l'habitant                   */
/**/   #define MAX_LOW 17000 /* Température maximale en absence */
/**/                         /* de l'habitant                   */
/**/                                                          /**/
/**/   int temperature = 0; /* variable où sera enregistrée la  */
/**/                        /*température                       */
/**/   int lumiere = 0; /* variable où sera enregistrée l'état  */
/**/                    /* de la luminosité (1/0)               */
/**/                    /* (il y a assez de lumière -> 1)       */
/**/                    /* (il n'y a pas assez de lumière -> 0) */
/**/   int magnetique = 0; /* variable où sera enregistrée la   */
/**/                       /* valeur du capteur magnétique (1/0)*/
/**/                       /* (l'aimant n'est pas détecté -> 1) */
/**/                       /* (l'aimant est détecté -> 0)       */
/**/   int presence = 0; /* il y a quelqu'un dans la maison -> 1*/
/**/                     /* il n'y a personne -> 0              */
/**/   int porteOuverte = 0; /* la porte est ouverte -> 1       */
/**/                         /* la porte est fermée -> 0        */
/**/   int chauffage = 0; /* le chauffage est allumé ->1        */
/**/                      /* le chaufage est éteint -> 0        */
/**/   int tableau[X][Y]; /* tableau où sont enregistrées des   */
/**/                      /* informations concernant les entrées*/
/**/                      /* et sorties de l'habitant :         */
/**/                      /*id, etat, heure d'arrivée et        */
/**/                      /* heure de départ                    */
/**/   int id = 0; /* nombres de fois où l'habitant a été       */
/**/               /* présent                                   */
/**/   int entrer = 0; /* est à 1 si l'utilisateur veut simuler */
/**/                   /* une entrée et à 0 dans le cas         */
/**/                   /* contraire                             */
/**/   int sortir = 0; /* est à 1 si l'utilisateur veut simuler */
/**/                   /* une sortie et à 0 dans le cas         */
/**/                   /* contraire                             */
/**/   int dormir = 0; /* est à 1 si l'habitant veut dormir et  */
/**/                   /* à 0 dans le cas contraire             */
/**/   int max = 0; /* en présence de l'habitant, est à 1       */
/**/                /* lorsque la température maximale est      */
/**/                /* et à 0 dlorsque la température minimale  */
/**/                /* est atteinte                             */
/**/   int maxLow = 0; /* en absence de l'habitant, est à 1     */
/**/                   /* lorsque la température maximale est   */
/**/                   /* atteint et à  lorsque la température  */
/**/                   /* minimale est atteinte                 */
/****************************************************************/


/*****************************************************************
* Cette fonction enregistre toutes les minutes la température    *
* dans un fichier.                                                    *
*****************************************************************/
void* tempLoger()
{
	while (1)
	{
		delay(60000);
		FILE* temp = NULL;
		temp = fopen("temperature.txt","a");
		fprintf(temp, "%d\n", temperature);
		fclose(temp);
	}
}

/*****************************************************************
* Cette fonction décide si il faut chauffer la maison ou pas.    *
*****************************************************************/
int chauffer(int temperature, int presence, int id, int tableau[X][Y])
{
	int i = 0;
	time_t maintenant;
	int compteur = 0;
	if (temperature == MAX)
		max = 1;
	if (max && temperature == MIN)
		max = 0;
	if (temperature == MAX_LOW)
		maxLow = 1;
	if (maxLow && temperature == MIN_LOW)
		maxLow = 0;
       if (temperature < 17000 && maxLow == 0) // maintient à une température minimale entre 17 et 15 degrée en absence de l'habitant
	{
		return 1; // Du chauffage est demandé
	}
	else if (temperature < MAX && max == 0 ) // maintient à une température comprise entre 20 et 22 degrés en présence de l'occupant ou lors de l'anticipation de son arrivée
	{
		if (presence)
		{
			return 1; // Du chauffage est demandé
		}
		maintenant = time(NULL);
		for (i = 0; i <= id; i++) // Le tableau d'entrées et sorties de l'habitant est parcouru
		{
			if (((maintenant + 1200)%86400 >= tableau[2][i]%86400) && ((maintenant + 1200)%86400 <= tableau[3][i]-((tableau[2][i]/86400)*86400))) // 12000 secondes = 20 minutes
			{
				++compteur; // Le compteur est incrémenté de 1 à chaque fois que l'habitant a été présent à l'heure qu'il sera dans une demi heure
			}
		}
		if ((tableau[2][id] - tableau[2][0]) != 0)
		{
			if ((compteur/((tableau[2][id-1] - tableau[2][0])/86400)) > 0.5 && id >= 1) // on vérifie qu'il a bien été présent au moins une fois sur deux
			{
				return 1; // Du chauffage est demandé
			}
		}
	}
	return 0; // On ne demande pas de chauffage
}

/*****************************************************************
* Cette fonction permet d'exporter le tableau d'entrées et       *
* sorties dans un fichier texte                                  *
*****************************************************************/
void exporter(int tableau[X][Y], int id)
{
	int i = 0;
	FILE* bdd = NULL;
	bdd = fopen("bdd.txt","w+");
	for (i = 0 ; i < id ; i++)
	{
		fprintf(bdd, "%d,", tableau[0][i]);
		fprintf(bdd, "%d,",tableau[1][i]);
		fprintf(bdd, "%d,", tableau[2][i]);
		fprintf(bdd, "%d\n", tableau[3][i]);
	}
	fclose(bdd);
}


/*****************************************************************
* Cette fonction permet d'importer un tableau d'entrées et de    *
* sorties contenues dans un fichier texte                        *
*****************************************************************/
void importer()
{
    char chaine[1000] = "";
    FILE* bdd = NULL;
    bdd = fopen("bdd.txt", "r+");

    if (bdd==NULL)
    {
        printf("erreur lors de la lecture de la bdd");
    }

    else
    {

        while (fscanf(bdd, "%d,%d,%d,%d\n", &tableau[0][id], &tableau[1][id], &tableau[2][id], &tableau[3][id]) != -1)
        {
            id++;
        }

    }

    fclose(bdd);

}




/*****************************************************************
* Cette fonction assure la gestion des capteurs. Elle lit la     *
* valeur de ces capteurs, initialise et appelle des fonctions en *
* conséquence des valeurs retournée par les capteurs.            *
*****************************************************************/
void* gestionCapteurs()
{
	time_t timestamp;
	int etatChffg = 0;
	int heure = 0;
	FILE* fichier = NULL;
	FILE* heureChauffage = NULL;
	system("sudo modprobe w1-gpio");
	system("sudo modprobe w1-therm");
	while (1) // Boucle infinie
	{
		delay(100); // Attente d'un dixième de seconde

		/********** Traitement de la température **********/
		fichier = fopen("/sys/bus/w1/devices/28-0000057b19e4/w1_slave","r");
		fseek(fichier, 69, SEEK_SET);
		fscanf(fichier,"%d",&temperature); // Lecture de la valeur de la température fournie par le capteur
		fclose(fichier);
		system("gpio mode 3 out");
		if (chauffer(temperature, presence, id, tableau) == 1) // Appel de la la fonction qui gère le chauffage
		{
			system("gpio write 3 0"); // Allumage du chauffage
			chauffage = 1;
			timestamp = time(NULL);
			if (etatChffg == 0) // Si le chauffage 'était pas encore allumé
			{
				heureChauffage = fopen("chauffage.txt","a");
				heure = timestamp;
				fprintf(heureChauffage, "début chauffage : %d\n",heure); // Ecriture dans un fichier de l'heure de déclenchement du chauffage
				fclose(heureChauffage);
			}
			etatChffg = 1;
		}
		else
		{
			system("gpio write 3 1"); // Extinction du chauffage
			chauffage = 0;
			timestamp = time(NULL);
			if (etatChffg == 1) // Si le chauffage était déjà allumé
			{
				heureChauffage = fopen("chauffage.txt","a");
				heure = timestamp;
				fprintf(heureChauffage, "fin chauffage : %d\n\n",heure); // Ecriture dans un fichier de l'heure d'arrêt du chauffage
				fclose(heureChauffage);
			}
			etatChffg = 0;
		}

		/********** Traitement de la lumière **********/
		if (dormir == 0)
		{
			system("cd /sys/class/gpio/ && echo 17 > export");
			fichier = fopen("/sys/class/gpio/gpio17/value","r");
			fscanf(fichier,"%d",&lumiere); // Lecture de la valeur de la luminosité fournie par le capteur
			fclose(fichier);
		}
		else if (dormir) // Si l'habitant veut dormir, on n'allume pas la lumière
		{
			lumiere = 0;
		}
		system("gpio mode 4 out");
		if (lumiere && presence)
		{
			system("gpio write 4 1"); // Allumage de la lumière
		}
		else
		{
			system("gpio write 4 0"); // Extinction de la lumière
		}

		/********** Traitement de l'état de la porte **********/
		system("cd /sys/class/gpio/ && echo 18 > export");
		fichier = fopen("/sys/class/gpio/gpio18/value","r");
		fscanf(fichier,"%d",&magnetique); // Lit la valeur du capteur magnétique
		fclose(fichier);
		if (magnetique == 1) // Si l'aimant ne se trouve plus devant le capteur
		{
			porteOuverte = 1;
			if (presence)
			{
				sortir = 1;
				entrer = 0;
			}
			else if (presence == 0)
			{
				entrer = 1;
				sortir = 0;
			}
		}
		else if (porteOuverte || entrer == 1 || sortir == 1) // Si la porte a été ouverte ou que l'habitant simule une entrée ou une sortie
		{
			if (presence == 0 && entrer == 1) // Si il n'y avait personne
			{
				tableau[0][id] = id;
				tableau[1][id] = 0;
				tableau[2][id] = time(NULL); // Les 3 dernières lignes remplissent le tableau
				presence = 1; // La présence passe à 1
				entrer = 0;
				sortir = 0;
			}
			else if (presence && sortir == 1) // Si il y avait quelqu'un
			{
				tableau[3][id] = time(NULL);
				tableau[1][id] = 1; // Ces deux lignes complètent le tableau
				++id;
				presence = 0; // La présence passe à 0
				sortir = 0;
				entrer = 0;
			}
			porteOuverte = 0;
		}
	}
}

/***********************************************************
* Cette fonction procure une interface de contrôle         *
* minimaliste en ligne de commande.                        *
***********************************************************/
void* shell()
{
	int continuer = 1;
	char commande[50], exit[]="exit";
	printf("Salut !\n");
	while (continuer == 1)
    {
		printf(">: ");
		scanf("%s", &commande); // Lecture de la commande
		if (strcmp(commande, exit) == 0) // Si la commande "exit" est saisie
        {
			printf("A bientôt !\n");
			system("gpio write 4 0"); // eteindre la lumiere
			lumiere = 0;
			system("gpio write 3 1"); // eteindre le chauffage
			chauffage = 0;
			printf("Exportation en cours ...\n");
			exporter(tableau, id);
			printf("Exportation réussie.\n");
			continuer = 0; // L'interface en ligne de commande se termine lorsque "exit" est entré. Cela rend l'exécution à la fonction principale
		}
		else if (strcmp(commande, "temperature") == 0)
		{
			printf("Temperature : %d°C\n", temperature);
		}
		else if (strcmp(commande, "presence") == 0)
		{
			printf("Presence : %d\n", presence);
		}
		else if (strcmp(commande, "chauffage") == 0)
		{
			printf("Chauffage : %d\n", chauffage);
		}
		else if (strcmp(commande, "exporter") == 0)
		{
			printf("Exportation en cours ...\n");
			exporter(tableau, id);
			printf("Exportation réussie.\n");
		}
		else if (strcmp(commande, "lumiere") == 0)
		{
			printf("Lumière : %d\n", lumiere);
		}
		else if (strcmp(commande, "entrer") == 0)
		{
			entrer = 1;
		}
		else if (strcmp(commande, "sortir") == 0)
		{
			sortir = 1;
		}
		else if (strcmp(commande, "dormir") == 0)
		{
			dormir = 1;
		}
		else if (strcmp(commande, "reveil") == 0)
		{
			dormir = 0;
		}
		else if (strcmp(commande, "importer") == 0)
		{
		    printf("Importation en cours ...\n");
			importer();
			printf("Importation réussie.\n");
		}
		else if (strcmp(commande, "id") == 0)
		{
			printf("id : %d\n", id);
		}
		else if (strcmp(commande, "aide") == 0)
		{
			FILE* fichier = NULL;
			char aide[83];
			fichier = fopen("aide.txt", "r");
			while (fgets(aide, 83, fichier) != NULL) // Affiche le fichier d'aide
			{
				printf("%s", aide);
			}
			fclose(fichier);
		}
	}
	return 0;
}

/**************************************************************
* C'est la fonction principale du programme. Elle assure      *
* le lancement et la synchronisation des différentes fonctions*
***************************************************************/
int main()
{
	time_t heure;
	heure = time(NULL);
	FILE* log = NULL;
	log = fopen("log.txt","w");
	fprintf(log, "début : %d\n", heure); // Enregistrement de l'heure de début du programme
	fclose(log);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t gestion, commande, tempLog;
	pthread_create(&gestion, &attr, gestionCapteurs, NULL); // Lancement de la fonction de gestion des capteurs
	pthread_create(&tempLog, &attr, tempLoger, NULL); // Lancement de la fonction d'enregistrement périodique de la tempétature
	pthread_create(&commande, NULL, shell, NULL); // Lancement de l'interface de contrôle
	pthread_join(commande, NULL); // On attend la fin du shell et la fonction principale se termine
	pthread_attr_destroy(&attr);
	system("gpio write 3 1"); // Extinction du chauffage
	system("gpio write 4 0"); // Extinction de la lumière
	heure = time(NULL);
	log = fopen("log.txt","w");
	fprintf(log, "fin : %d\n", heure);
	fclose(log);
	return 0;
}
