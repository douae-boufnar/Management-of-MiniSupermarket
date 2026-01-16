#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// --- DEFINITIONS ET CONSTANTES ---
#define TAILLE_TABLE 13
#define MAX_NOM 30
#define FICHIER_PRODUITS "produits.txt"
#define FICHIER_CLIENTS "clients.txt"
#define FICHIER_HISTORIQUE "historique.txt"

// --- 1. STRUCTURES DE DONNÉES ---

// 1.1 Structure Produit
typedef struct Produit {
    int id;
    char nom[MAX_NOM];
    float prix;
    int quantite;
    struct Produit *suivant;
} Produit;

// 1.2 Table de Hachage
typedef struct {
    Produit *table[TAILLE_TABLE];
} TableHachage;

// 1.3 Structure Client (ABR)
typedef struct Client {
    int id;
    char nom[30];
    float totalDepense;
    struct Client *gauche;
    struct Client *droite;
} client;

typedef struct arbre {
    client *racine;
} arbre;

// 1.4 File d'Attente
typedef struct ClientFile {
    int idClient;
    struct ClientFile *suivant;
} ClientFile;

typedef struct {
    ClientFile *tete;
    ClientFile *queue;
    int nef;
} FileAttente;

// 1.5 Structure Transaction (Pile - Historique)
// AJOUT : nomProduit et date pour l'affichage demandé
typedef struct Transaction {
    int idClient;
    int idProduit;
    char nomProduit[MAX_NOM];
    int quantite;
    float total;
    char date[30];
    struct Transaction *suivant;
} Transaction;

// 1.6 Helper pour le panier (Liste chainée temporaire avant validation)
typedef struct PanierItem {
    int idProduit;
    char nomProduit[MAX_NOM];
    int quantite;
    float prixUnitaire;
    float totalLigne;
    struct PanierItem *suivant;
} PanierItem;

// --- VARIABLES GLOBALES ---
TableHachage magasin;
arbre ArbreClients;
FileAttente FileCaisse;
Transaction *PileHistorique = NULL;

// --- FONCTION UTILITAIRE : DATE ---
void obtenirDateActuelle(char *buffer) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    // Format : JJ/MM/AAAA HH:MM
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
            tm.tm_hour, tm.tm_min);
}

// --- 2. GESTION PRODUITS (Hachage) ---

int hachage(int id) {
    return id % TAILLE_TABLE;
}

void initialiserTable() {
    for (int i = 0; i < TAILLE_TABLE; i++) magasin.table[i] = NULL;
}

Produit* rechercherProduitParID(int id) {
    int index = hachage(id);
    Produit *courant = magasin.table[index];
    while (courant != NULL) {
        if (courant->id == id) return courant;
        courant = courant->suivant;
    }
    return NULL;
}

void ajouterProduit(int id, char *nom, float prix, int quantite, int silencieux) {
    if (rechercherProduitParID(id) != NULL) {
        if(!silencieux) printf("Erreur : L'ID %d existe deja.\n", id);
        return;
    }
    Produit *nouveau = (Produit*)malloc(sizeof(Produit));
    nouveau->id = id;
    strcpy(nouveau->nom, nom);
    nouveau->prix = prix;
    nouveau->quantite = quantite;

    int index = hachage(id);
    nouveau->suivant = magasin.table[index];
    magasin.table[index] = nouveau;

    if(!silencieux) printf("Produit ajoute : %s (ID: %d)\n", nom, id);
}

void modifierProduit(int id) {
    Produit *p = rechercherProduitParID(id);
    if (p == NULL) { printf("Produit introuvable.\n"); return; }

    printf("Actuel -> Prix: %.2f | Stock: %d\n", p->prix, p->quantite);
    printf("Nouveau prix : "); scanf("%f", &p->prix);
    printf("Nouvelle quantite : "); scanf("%d", &p->quantite);
    printf("Produit mis a jour.\n");
}

void supprimerProduit(int id) {
    int index = hachage(id);
    Produit *courant = magasin.table[index];
    Produit *precedent = NULL;

    while (courant != NULL) {
        if (courant->id == id) {
            if (precedent == NULL) magasin.table[index] = courant->suivant;
            else precedent->suivant = courant->suivant;
            free(courant);
            printf("Produit ID %d supprime.\n", id);
            return;
        }
        precedent = courant;
        courant = courant->suivant;
    }
    printf("Erreur : Produit non trouve.\n");
}

int comparerProduits(const void *a, const void *b) {
    Produit *p1 = *(Produit**)a;
    Produit *p2 = *(Produit**)b;
    return strcmp(p1->nom, p2->nom);
}

void afficherProduits(int trie) {
    printf("\n=== LISTE DES PRODUITS ===\n");
    printf("ID\t| Nom\t\t| Prix\t| Stock\n");
    printf("----------------------------------------\n");

    if (!trie) {
        for (int i = 0; i < TAILLE_TABLE; i++) {
            Produit *p = magasin.table[i];
            while (p != NULL) {
                printf("%d\t| %-15s| %.2f\t| %d\n", p->id, p->nom, p->prix, p->quantite);
                p = p->suivant;
            }
        }
    } else {
        // Logique de tri simple pour l'affichage
        int count = 0;
        for (int i = 0; i < TAILLE_TABLE; i++) {
            Produit *p = magasin.table[i];
            while (p) { count++; p = p->suivant; }
        }
        if(count == 0) return;

        Produit **tab = malloc(count * sizeof(Produit*));
        int k=0;
        for (int i = 0; i < TAILLE_TABLE; i++) {
            Produit *p = magasin.table[i];
            while (p) { tab[k++] = p; p = p->suivant; }
        }
        qsort(tab, count, sizeof(Produit*), comparerProduits);
        for(int i=0; i<count; i++)
            printf("%d\t| %-15s| %.2f\t| %d\n", tab[i]->id, tab[i]->nom, tab[i]->prix, tab[i]->quantite);
        free(tab);
    }
    printf("----------------------------------------\n");
}

void sauvegarderProduits() {
    FILE *f = fopen("C://Users//PC//produits.txt", "w");
    if (!f) return;
    for (int i = 0; i < TAILLE_TABLE; i++) {
        Produit *p = magasin.table[i];
        while (p != NULL) {
            fprintf(f, "%d|%s|%.2f|%d\n", p->id, p->nom, p->prix, p->quantite);
            p = p->suivant;
        }
    }
    fclose(f);
    printf("Produits sauvegardes.\n");
}

void chargerProduits() {
    FILE *f = fopen("C://Users//PC//produits.txt", "r");
    if (!f) return;
    char ligne[256];
    while (fgets(ligne, sizeof(ligne), f)) {
        char *token = strtok(ligne, "|"); if(!token) continue;
        int id = atoi(token);
        token = strtok(NULL, "|"); if(!token) continue;
        char nom[MAX_NOM]; strcpy(nom, token);
        token = strtok(NULL, "|"); if(!token) continue;
        float prix = atof(token);
        token = strtok(NULL, "|"); if(!token) continue;
        int qte = atoi(token);
        ajouterProduit(id, nom, prix, qte, 1); // 1 = mode silencieux
    }
    fclose(f);
    printf("Produits charges.\n");
}

// --- 3. GESTION CLIENTS (ABR) ---

client* Creer_client(int id, char* nom, float total) {
    client *C = (client*)malloc(sizeof(client));
    C->id = id; strcpy(C->nom, nom); C->totalDepense = total;
    C->gauche = NULL; C->droite = NULL;
    return C;
}

client* insertion_client(client *racine, client *C) {
    if (racine == NULL) return C;
    if (strcmp(C->nom, racine->nom) < 0)
        racine->gauche = insertion_client(racine->gauche, C);
    else if (strcmp(C->nom, racine->nom) > 0)
        racine->droite = insertion_client(racine->droite, C);
    else
        printf("Le client %s existe deja (Nom doublon).\n", C->nom);
    return racine;
}

void parcours_infixe(client* racine){
    if(racine != NULL){
        parcours_infixe(racine->gauche);
        printf("ID: %-5d | Nom: %-15s | Total: %.2f\n", racine->id, racine->nom, racine->totalDepense);
        parcours_infixe(racine->droite);
    }
}

client* recherche_client_par_nom(client *racine, const char *nom) {
    if (racine == NULL) return NULL;
    int cmp = strcmp(nom, racine->nom);
    if (cmp == 0) return racine;
    if (cmp < 0) return recherche_client_par_nom(racine->gauche, nom);
    return recherche_client_par_nom(racine->droite, nom);
}

client* recherche_client_par_id_abr(client *racine, int id) {
    if (racine == NULL) return NULL;
    if (racine->id == id) return racine;
    client *res = recherche_client_par_id_abr(racine->gauche, id);
    if (res) return res;
    return recherche_client_par_id_abr(racine->droite, id);
}

client* minValue(client *r) {
    while (r->gauche != NULL) r = r->gauche;
    return r;
}

client* supprimer_nom(client *r, char nom[]) {
    if (r == NULL) return NULL;
    int cmp = strcmp(nom, r->nom);
    if (cmp < 0) r->gauche = supprimer_nom(r->gauche, nom);
    else if (cmp > 0) r->droite = supprimer_nom(r->droite, nom);
    else {
        if (r->gauche == NULL) { client *temp = r->droite; free(r); return temp; }
        else if (r->droite == NULL) { client *temp = r->gauche; free(r); return temp; }
        client *succ = minValue(r->droite);
        r->id = succ->id; strcpy(r->nom, succ->nom); r->totalDepense = succ->totalDepense;
        r->droite = supprimer_nom(r->droite, succ->nom);
    }
    return r;
}

void parcours_infixe_fichier(client *racine, FILE *fp) {
    if (racine != NULL) {
        parcours_infixe_fichier(racine->gauche, fp);
        fprintf(fp, "%d|%s|%.2f\n", racine->id, racine->nom, racine->totalDepense);
        parcours_infixe_fichier(racine->droite, fp);
    }
}

void sauvegarder_clients(arbre *A) {
    FILE *fp = fopen("C://Users//PC//clients.txt", "w");
    if (!fp) return;
    parcours_infixe_fichier(A->racine, fp);
    fclose(fp);
    printf("Clients sauvegardes.\n");
}

void lire_clients(arbre *A) {
    FILE *fp = fopen("C://Users//PC//clients.txt", "r");
    if (!fp) return;
    char ligne[100];
    int id; char nom[30]; float total;
    while (fgets(ligne, sizeof(ligne), fp)) {
         char *p = strtok(ligne, "|"); if(!p) continue; id = atoi(p);
         p = strtok(NULL, "|"); if(!p) continue; strcpy(nom, p);
         p = strtok(NULL, "|"); if(!p) continue; total = atof(p);

         // Vérif doublon ID pour éviter le spam "Le client existe deja"
         if(recherche_client_par_id_abr(A->racine, id) == NULL) {
             client *n = Creer_client(id, nom, total);
             A->racine = insertion_client(A->racine, n);
         }
    }
    fclose(fp);
    printf("Clients charges.\n");
}

// --- 4. FILE ATTENTE ---
void initialiserFile(FileAttente *Fi) { Fi->tete = NULL; Fi->queue = NULL; Fi->nef = 0; }
int estVideFile(FileAttente *f) { return f->nef == 0; }
void enfiler(FileAttente *f, int idClient) {
    ClientFile *c = (ClientFile*)malloc(sizeof(ClientFile));
    c->idClient = idClient; c->suivant = NULL;
    if (f->tete == NULL) { f->tete = c; f->queue = c; }
    else { f->queue->suivant = c; f->queue = c; }
    f->nef++;
    printf("Client ID %d ajoute a la file.\n", idClient);
}
int defiler(FileAttente *Fi) {
    if (estVideFile(Fi)) return -1;
    ClientFile *tmp = Fi->tete; int id = tmp->idClient;
    Fi->tete = Fi->tete->suivant; Fi->nef--;
    if (Fi->tete == NULL) Fi->queue = NULL;
    free(tmp); return id;
}
void afficherFile(FileAttente *f) {
    printf("\n--- File d'attente (%d) ---\n", f->nef);
    ClientFile *ptr = f->tete;
    while(ptr) {
        client *c = recherche_client_par_id_abr(ArbreClients.racine, ptr->idClient);
        if(c) printf(" - [%d] %s\n", c->id, c->nom);
        else printf(" - [%d] Inconnu\n", ptr->idClient);
        ptr = ptr->suivant;
    }
}

// --- 5. HISTORIQUE ---

void push_transaction(Transaction **pile, int idC, int idP, char *nomP, int q, float total, const char *date) {
    Transaction *t = (Transaction*)malloc(sizeof(Transaction));
    t->idClient = idC; t->idProduit = idP;
    strcpy(t->nomProduit, nomP);
    t->quantite = q; t->total = total;
    strcpy(t->date, date);
    t->suivant = *pile;
    *pile = t;
}

int pop_transaction(Transaction **pile) {
    if (*pile == NULL) { printf("Historique vide.\n"); return 0; }
    Transaction *tmp = *pile;
    *pile = tmp->suivant;
    printf("Transaction annulee (Historique): Client %d | Produit %s | Total %.2f\n", tmp->idClient, tmp->nomProduit, tmp->total);
    // Note: Stock non remis automatiquement ici pour simplicité, mais idéalement il le faudrait
    free(tmp);
    return 1;
}

void afficherHistorique(Transaction *pile) {
    printf("\n============================ HISTORIQUE DES VENTES ============================\n");
    printf("| %-10s | %-10s | %-15s | %-5s | %-16s | %-8s |\n", "ID CLIENT", "ID PROD", "NOM PROD", "QTE", "DATE", "TOTAL");
    printf("-------------------------------------------------------------------------------\n");
    while (pile != NULL) {
        printf("| %-10d | %-10d | %-15s | %-5d | %-16s | %-8.2f |\n",
               pile->idClient, pile->idProduit, pile->nomProduit, pile->quantite, pile->date, pile->total);
        pile = pile->suivant;
    }
    printf("===============================================================================\n");
}

void sauvegarderHistorique(Transaction *pile) {
    FILE *f = fopen("C://Users//PC//historique.txt", "w");
    if (!f) return;
    while (pile != NULL) {
        // Format: idC|idP|NomP|Qte|Total|Date
        fprintf(f, "%d|%d|%s|%d|%.2f|%s\n", pile->idClient, pile->idProduit, pile->nomProduit, pile->quantite, pile->total, pile->date);
        pile = pile->suivant;
    }
    fclose(f);
    printf("Historique sauvegarde.\n");
}

void chargerHistorique(Transaction **pile) {
    FILE *f = fopen("C://Users//PC//historique.txt", "r");
    if (!f) return;
    char ligne[256];
    while (fgets(ligne, sizeof(ligne), f)) {
        ligne[strcspn(ligne, "\n")] = 0;
        char *token = strtok(ligne, "|"); if(!token) continue; int idC = atoi(token);
        token = strtok(NULL, "|"); if(!token) continue; int idP = atoi(token);
        token = strtok(NULL, "|"); if(!token) continue; char nomP[30]; strcpy(nomP, token);
        token = strtok(NULL, "|"); if(!token) continue; int q = atoi(token);
        token = strtok(NULL, "|"); if(!token) continue; float tot = atof(token);
        token = strtok(NULL, "|"); if(!token) continue; char date[30]; strcpy(date, token);

        push_transaction(pile, idC, idP, nomP, q, tot, date);
    }
    fclose(f);
    printf("Historique charge.\n");
}

// --- 6. CAISSE & PANIER (LOGIQUE COMPLEXE) ---

void genererTicket(int idClient, PanierItem *panier, float total, char* date) {
    char nomFichier[100];
    sprintf(nomFichier, "ticket_%d.txt", idClient);
    FILE *f = fopen("C://Users//PC//ticket_%d.txt", "w");
    if (!f) return;

    fprintf(f, "======= TICKET DE CAISSE =======\n");
    fprintf(f, "Client ID : %d\n", idClient);
    fprintf(f, "Date      : %s\n", date);
    fprintf(f, "--------------------------------\n");
    PanierItem *tmp = panier;
    while (tmp) {
        fprintf(f, "%-15s x%d : %.2f\n", tmp->nomProduit, tmp->quantite, tmp->totalLigne);
        tmp = tmp->suivant;
    }
    fprintf(f, "--------------------------------\n");
    fprintf(f, "TOTAL: %.2f DH\n", total);
    fclose(f);
    printf("Ticket sauvegarde dans %s\n", nomFichier);
}

void traiterAchat(int idClient) {
    client *cl = recherche_client_par_id_abr(ArbreClients.racine, idClient);
    if (!cl) { printf("Client introuvable.\n"); return; }

    printf("\n--- Panier pour %s (ID: %d) ---\n", cl->nom, cl->id);

    PanierItem *panier = NULL;
    int terminer = 0;

    // Boucle d'achat interactif (Ajout/Modif/Annulation)
    while (!terminer) {
        printf("\n1. Ajouter un produit\n");
        printf("2. Modifier quantite ou Annuler (Mettre 0)\n");
        printf("3. Valider le panier et Payer\n");
        printf("0. Tout annuler et sortir\n");
        printf("Votre choix : ");
        int choix; scanf("%d", &choix);

        if (choix == 1) { // AJOUT
            int idP;
            printf("ID Produit : "); scanf("%d", &idP);
            Produit *p = rechercherProduitParID(idP);
            if (!p) { printf("Produit inconnu.\n"); continue; }

            printf("Produit: %s | Prix: %.2f | Stock: %d\n", p->nom, p->prix, p->quantite);
            printf("Quantite : ");
            int qte; scanf("%d", &qte);

            if (qte > p->quantite) {
                printf("Stock insuffisant !\n");
            } else if (qte > 0) {
                // Vérifier si déjà dans panier
                PanierItem *it = panier;
                int trouve = 0;
                while(it) {
                    if (it->idProduit == idP) {
                        if (it->quantite + qte > p->quantite) printf("Stock depasse (cumul) !\n");
                        else {
                            it->quantite += qte;
                            it->totalLigne = it->quantite * it->prixUnitaire;
                            printf("Quantite mise a jour dans le panier.\n");
                        }
                        trouve = 1; break;
                    }
                    it = it->suivant;
                }
                if (!trouve) {
                    PanierItem *n = (PanierItem*)malloc(sizeof(PanierItem));
                    n->idProduit = idP; strcpy(n->nomProduit, p->nom);
                    n->quantite = qte; n->prixUnitaire = p->prix;
                    n->totalLigne = p->prix * qte;
                    n->suivant = panier; panier = n;
                    printf("Ajoute au panier.\n");
                }
            }
        }
        else if (choix == 2) { // MODIFICATION / ANNULATION
            if(!panier) { printf("Panier vide.\n"); continue; }
            printf("\n-- Contenu Panier --\n");
            PanierItem *it = panier;
            while(it) { printf("ID %d : %s (x%d)\n", it->idProduit, it->nomProduit, it->quantite); it = it->suivant; }

            printf("ID du produit a modifier : ");
            int idM; scanf("%d", &idM);

            PanierItem *courant = panier;
            PanierItem *prec = NULL;
            int found = 0;
            while(courant) {
                if(courant->idProduit == idM) {
                    found = 1;
                    printf("Nouvelle quantite (0 pour supprimer) : ");
                    int nq; scanf("%d", &nq);
                    Produit *stockReel = rechercherProduitParID(idM);

                    if (nq == 0) {
                        // Suppression
                        if(prec == NULL) panier = courant->suivant;
                        else prec->suivant = courant->suivant;
                        free(courant);
                        printf("Article retire du panier.\n");
                    } else if (nq > stockReel->quantite) {
                        printf("Stock insuffisant.\n");
                    } else {
                        courant->quantite = nq;
                        courant->totalLigne = nq * courant->prixUnitaire;
                        printf("Quantite modifiee.\n");
                    }
                    break;
                }
                prec = courant;
                courant = courant->suivant;
            }
            if(!found) printf("Produit absent du panier.\n");
        }
        else if (choix == 3) { // VALIDATION
            terminer = 1;
        }
        else if (choix == 0) { // ABANDON
            while(panier) { PanierItem *t = panier; panier = panier->suivant; free(t); }
            printf("Achat annule.\n");
            return;
        }
    }

    if (!panier) { printf("Panier vide.\n"); return; }

    // Traitement final
    float totalGlobal = 0;
    char dateNow[30];
    obtenirDateActuelle(dateNow);

    PanierItem *it = panier;
    while (it) {
        Produit *p = rechercherProduitParID(it->idProduit);
        if (p) p->quantite -= it->quantite; // Décrémenter stock

        push_transaction(&PileHistorique, idClient, it->idProduit, it->nomProduit, it->quantite, it->totalLigne, dateNow);
        totalGlobal += it->totalLigne;

        it = it->suivant;
    }

    cl->totalDepense += totalGlobal;
    genererTicket(idClient, panier, totalGlobal, dateNow);
    printf("\n>>> PAIEMENT VALIDE. TOTAL: %.2f DH <<<\n", totalGlobal);

    // Libération mémoire panier
    while(panier) { PanierItem *t = panier; panier = panier->suivant; free(t); }
}

// --- 7. MENUS (IDENTIQUES AU CODE INITIAL) ---

void MenuProduits() {
    int choix;
    do {
        printf("\n--- Gestion des produits ---\n");
        printf("1) Ajouter produit\n");
        printf("2) Modifier produit\n");
        printf("3) Supprimer produit\n");
        printf("4) Rechercher par ID\n");
        printf("5) Rechercher par nom (Non impl)\n");
        printf("6) Lister produits (Trie/Non Trie)\n");
        printf("7) Sauvegarder\n");
        printf("8) Charger\n");
        printf("0) Retour\n");
        printf("Choix: ");
        scanf("%d", &choix);

        int id, qte; char nom[MAX_NOM]; float prix;
        switch(choix) {
            case 1:
                printf("ID: "); scanf("%d", &id);
                printf("Nom: "); scanf("%s", nom);
                printf("Prix: "); scanf("%f", &prix);
                printf("Qte: "); scanf("%d", &qte);
                ajouterProduit(id, nom, prix, qte, 0); // 0 = verbeux
                break;
            case 2: printf("ID a modifier: "); scanf("%d", &id); modifierProduit(id); break;
            case 3: printf("ID a supprimer: "); scanf("%d", &id); supprimerProduit(id); break;
            case 4:
                printf("ID: "); scanf("%d", &id);
                Produit *p = rechercherProduitParID(id);
                if(p) printf("Trouve: %s | %.2f | qte=%d\n", p->nom, p->prix, p->quantite);
                else printf("Non trouve.\n");
                break;
            case 5:
                printf("Nom du produit a rechercher : ");
                scanf("%s", nom); // Variable 'nom' déjà déclarée au début du switch

                int trouve = 0;
                printf("\n--- RESULTATS DE RECHERCHE ---\n");

                // On doit parcourir toute la table de hachage (tous les index)
                for (int i = 0; i < TAILLE_TABLE; i++) {
                    Produit *p = magasin.table[i];
                    while (p != NULL) {
                        // Comparaison de chaînes (strcmp renvoie 0 si identique)
                        if (strcmp(p->nom, nom) == 0) {
                            printf("-> ID: %d | Nom: %s | Prix: %.2f | Stock: %d\n",
                                   p->id, p->nom, p->prix, p->quantite);
                            trouve = 1;
                        }
                        p = p->suivant;
                    }
                }

                if (!trouve) {
                    printf("Aucun produit trouve avec le nom '%s'.\n", nom);
                }
                printf("------------------------------\n");
                break;
            case 6: printf("1=Trie, 0=Non trie : "); int t; scanf("%d", &t); afficherProduits(t); break;
            case 7: sauvegarderProduits(); break;
            case 8: chargerProduits(); break;
        }
    } while(choix != 0);
}

void MenuClients() {
    int choix;
    do {
        printf("\n--- Gestion des clients ---\n");
        printf("1) Ajouter client\n");
        printf("2) Rechercher par nom\n");
        printf("3) Rechercher par ID\n");
        printf("4) Supprimer client (par nom)\n");
        printf("5) Lister clients (alphabetique)\n");
        printf("6) Sauvegarder\n");
        printf("7) Charger\n");
        printf("8) Mettre en file d'attente\n");
        printf("9) Afficher file\n");
        printf("0) Retour\n");
        printf("Choix: ");
        scanf("%d", &choix);

        char nom[30]; int id;
        switch(choix) {
            case 1:
                printf("ID: "); scanf("%d", &id);
                if(recherche_client_par_id_abr(ArbreClients.racine, id)) {
                    printf("Cet ID existe deja.\n");
                } else {
                    float tot;
                    printf("Nom: "); scanf(" %[^\n]", nom);
                    printf("Total: "); scanf("%f", &tot);
                    client* nc = Creer_client(id, nom, tot);
                    ArbreClients.racine = insertion_client(ArbreClients.racine, nc);
                }
                break;
            case 2:
                printf("Nom: "); scanf("%s", nom);
                client *res = recherche_client_par_nom(ArbreClients.racine, nom);
                if(res) printf("ID: %d, Total: %.2f\n", res->id, res->totalDepense);
                else printf("Inconnu.\n");
                break;
           case 3:
                printf("ID: "); scanf("%d", &id);
                client *clId = recherche_client_par_id_abr(ArbreClients.racine, id);
                if (clId) printf("Trouve: %s (Total: %.2f)\n", clId->nom, clId->totalDepense);
                else printf("Inconnu.\n");
                break;
            case 4: printf("Nom: "); scanf("%s", nom); ArbreClients.racine = supprimer_nom(ArbreClients.racine, nom); break;
            case 5: printf("\n--- LISTE CLIENTS ---\n"); parcours_infixe(ArbreClients.racine); break;
            case 6: sauvegarder_clients(&ArbreClients); break;
            case 7: lire_clients(&ArbreClients); break;
            case 8:
                printf("ID Client: "); scanf("%d", &id);
                if(recherche_client_par_id_abr(ArbreClients.racine, id)) enfiler(&FileCaisse, id);
                else printf("Client inconnu.\n");
                break;
            case 9: afficherFile(&FileCaisse); break;
        }
    } while(choix != 0);
}

void MenuCaisse() {
    int choix;
    do {
        printf("\n--- Passage en caisse ---\n");
        printf("1) Servir prochain client\n");
        printf("2) Ajouter panier manuellement (Direct ID)\n");
        printf("3) Afficher historique\n");
        printf("4) Annuler derniere transaction\n");
        printf("5) Sauvegarder historique\n");
        printf("0) Retour\n");
        printf("Choix: ");
        scanf("%d", &choix);

        switch(choix) {
            case 1:
                if (!estVideFile(&FileCaisse)) {
                    int id = defiler(&FileCaisse);
                    traiterAchat(id);
                } else printf("Personne dans la file.\n");
                break;
            case 2: printf("ID Client: "); int id; scanf("%d", &id); traiterAchat(id); break;
            case 3: afficherHistorique(PileHistorique); break;
            case 4: pop_transaction(&PileHistorique); break;
            case 5: sauvegarderHistorique(PileHistorique); break;
        }
    } while(choix != 0);
}

int main() {
    // Initialisation
    initialiserTable();
    ArbreClients.racine = NULL;
    initialiserFile(&FileCaisse);

    // CHARGEMENT AUTOMATIQUE
    printf("Chargement des donnees...\n");
    chargerProduits();
    lire_clients(&ArbreClients);
    chargerHistorique(&PileHistorique);

    // AFFICHAGE AUTOMATIQUE AU DEBUT
    afficherProduits(0);
    printf("\n--- LISTE CLIENTS ---\n");
    parcours_infixe(ArbreClients.racine);

    int choix;
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1) Gestion produits\n");
        printf("2) Gestion clients\n");
        printf("3) Passage en caisse\n");
        printf("4) Sauvegarder tout\n");
        printf("5) Charger tout\n");
        printf("0) Quitter\n");
        printf("Choix: ");
        scanf("%d", &choix);

        switch(choix) {
            case 1: MenuProduits(); break;
            case 2: MenuClients(); break;
            case 3: MenuCaisse(); break;
            case 4:
                sauvegarderProduits();
                sauvegarder_clients(&ArbreClients);
                sauvegarderHistorique(PileHistorique);
                break;
            case 5:
                chargerProduits();
                ArbreClients.racine = NULL; // Reset pour éviter doublons
                lire_clients(&ArbreClients);
                chargerHistorique(&PileHistorique);
                // Re-affichage après chargement manuel
                afficherProduits(0);
                printf("\n--- LISTE CLIENTS ---\n");
                parcours_infixe(ArbreClients.racine);
                break;
            case 0: printf("Au revoir!\n"); break;
            default: printf("Choix invalide.\n");
        }
    } while(choix != 0);

    return 0;
}
