#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "sqlite3.h"

/* --- Yapý Tanýmlarý --- */
typedef struct Course {
    char courseName[50];
    double yazili1, yazili2, sozlu, average, participation_index;
    char ai_comment[150];
    struct Course *nextCourse;
} course;

typedef struct Student {
    int no;
    char firstName[30], lastName[30];
    double gpa;
    char academic_standing[100], achievement_badge[50], star_course[50];
    struct Course *courses;
    struct Student *next;
} std;

/* --- Yardýmcý Fonksiyonlar --- */
void stringToUpper(char *str) {
	int i;
    for (i = 0; str[i]; i++) str[i] = toupper((unsigned char)str[i]);
}

double getValidGrade(char *gradeType) {
    double grade;
    while (1) {
        printf("    %s Notu (0-100): ", gradeType);
        if (scanf("%lf", &grade) != 1) {
            printf("      [Hata] Lutfen sayi giriniz!\n");
            while (getchar() != '\n'); continue;
        }
        if (grade >= 0 && grade <= 100) return grade;
        printf("      [Hata] Not 0-100 araliginda olmalidir!\n");
    }
}

/* --- AI Analiz Motorlarý --- */
void aiCourseAnalyzer(course *c) {
    c->average = (c->yazili1 + c->yazili2 + c->sozlu) / 3.0;
    c->participation_index = c->sozlu - ((c->yazili1 + c->yazili2) / 2.0);
    if (c->average >= 50.0) {
        strcpy(c->ai_comment, (c->participation_index > 15) ? "Derse Katilim Harika" : "Dengeli Performans");
    } else strcpy(c->ai_comment, "Zayif: Tekrar ve Destek Gerekli");
}

void aiStudentStatusAnalyzer(std *s) {
    if (!s->courses) return;
    double totalAvg = 0, maxPart = -999.0;
    int count = 0;
    course *temp = s->courses;
    while (temp) {
        totalAvg += temp->average;
        if (temp->participation_index > maxPart) {
            maxPart = temp->participation_index;
            strcpy(s->star_course, temp->courseName);
        }
        count++; temp = temp->nextCourse;
    }
    s->gpa = (count > 0) ? (totalAvg / count) : 0;
    if (s->gpa >= 85) { strcpy(s->academic_standing, "Takdir Adayi"); strcpy(s->achievement_badge, "ALTIN"); }
    else if (s->gpa >= 70) { strcpy(s->academic_standing, "Tesekkur Adayi"); strcpy(s->achievement_badge, "GUMUS"); }
    else { strcpy(s->academic_standing, "Gecer/Riskli"); strcpy(s->achievement_badge, "BRONZ"); }
}

void aiCareerAdvisor(std *s) {
    if (!s->courses) return;
    double sayPuan = 0, sozPuan = 0;
    int sayC = 0, sozC = 0;
    course *c = s->courses;
    while (c) {
        if (strstr(c->courseName, "MAT") || strstr(c->courseName, "FIZ") || strstr(c->courseName, "FEN") || strstr(c->courseName, "KIM")) {
            sayPuan += c->average; sayC++;
        } else if (strstr(c->courseName, "TURK") || strstr(c->courseName, "TAR") || strstr(c->courseName, "COG") || strstr(c->courseName, "EDEB")) {
            sozPuan += c->average; sozC++;
        }
        c = c->nextCourse;
    }
    double sayAvg = (sayC > 0) ? (sayPuan / sayC) : 0;
    double sozAvg = (sozC > 0) ? (sozPuan / sozC) : 0;
    double fark = sayAvg - sozAvg;

    printf("\n=== KARÝYER REHBERÝ: %s %s ===", s->firstName, s->lastName);
    if (fabs(fark) <= 10.0 && sayAvg >= 60 && sozAvg >= 60) {
        printf("\nAlan: ESIT AGIRLIK (EA)\nOnerilen: Hukuk, Psikoloji, Isletme veya Iktisat.");
    } else if (sayAvg > sozAvg) {
        printf("\nAlan: SAYISAL (SAY)\nOnerilen: Tip Fakultesi, Muhendislik (Yazilim/Makine).");
    } else {
        printf("\nAlan: SOZEL (SOZ)\nOnerilen: Turkce/Sosyal Bilgiler Ogretmenligi, Gazetecilik.");
    }
    printf("\n===========================================\n");
}

/* --- Görselleþtirme (ASCII Grafik) --- */
void displayPerformanceGraph(std *s) {
    if (!s || !s->courses) return;
    printf("\n--- BASARI GRAFIGI (%s %s) ---", s->firstName, s->lastName);
    course *c = s->courses;
    while (c) {
        printf("\n%-12s [%5.2f] | ", c->courseName, c->average);
        int bar = (int)(c->average / 5);
        int i;
        for (i = 0; i < bar; i++) printf("-");
        c = c->nextCourse;
    }
    printf("\n-------------------------------------------\n");
}

/* --- SQLite Veritabaný Ýþlemleri --- */
void initDB() {
    sqlite3 *db;
    sqlite3_open("akademik.db", &db);
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS Students(No INT PRIMARY KEY, Ad TEXT, Soyad TEXT, GPA REAL);"
                     "CREATE TABLE IF NOT EXISTS Courses(StdNo INT, Ad TEXT, Y1 REAL, Y2 REAL, S REAL, Ort REAL);", 0, 0, 0);
    sqlite3_close(db);
}

void syncDB(std *head) {
    sqlite3 *db;
    sqlite3_open("akademik.db", &db);
    sqlite3_exec(db, "DELETE FROM Students; DELETE FROM Courses;", 0, 0, 0);
    while (head) {
        char sql[512];
        sprintf(sql, "INSERT INTO Students VALUES(%d, '%s', '%s', %f);", head->no, head->firstName, head->lastName, head->gpa);
        sqlite3_exec(db, sql, 0, 0, 0);
        course *c = head->courses;
        while (c) {
            sprintf(sql, "INSERT INTO Courses VALUES(%d, '%s', %f, %f, %f, %f);", head->no, c->courseName, c->yazili1, c->yazili2, c->sozlu, c->average);
            sqlite3_exec(db, sql, 0, 0, 0);
            c = c->nextCourse;
        }
        head = head->next;
    }
    sqlite3_close(db);
}

std* loadDB() {
    sqlite3 *db; sqlite3_stmt *res; std *head = NULL;
    if (sqlite3_open("akademik.db", &db) != SQLITE_OK) return NULL;
    sqlite3_prepare_v2(db, "SELECT * FROM Students", -1, &res, 0);
    while (sqlite3_step(res) == SQLITE_ROW) {
        std *ns = (std*)malloc(sizeof(std));
        ns->no = sqlite3_column_int(res, 0);
        strcpy(ns->firstName, (const char*)sqlite3_column_text(res, 1));
        strcpy(ns->lastName, (const char*)sqlite3_column_text(res, 2));
        ns->courses = NULL; ns->next = head; head = ns;
    }
    sqlite3_finalize(res);
    std *tS = head;
    while (tS) {
        char sql[128]; sprintf(sql, "SELECT Ad, Y1, Y2, S, Ort FROM Courses WHERE StdNo = %d", tS->no);
        sqlite3_prepare_v2(db, sql, -1, &res, 0);
        while (sqlite3_step(res) == SQLITE_ROW) {
            course *nc = (course*)malloc(sizeof(course));
            strcpy(nc->courseName, (const char*)sqlite3_column_text(res, 0));
            nc->yazili1 = sqlite3_column_double(res, 1); nc->yazili2 = sqlite3_column_double(res, 2);
            nc->sozlu = sqlite3_column_double(res, 3); nc->average = sqlite3_column_double(res, 4);
            aiCourseAnalyzer(nc); nc->nextCourse = tS->courses; tS->courses = nc;
        }
        sqlite3_finalize(res); aiStudentStatusAnalyzer(tS); tS = tS->next;
    }
    sqlite3_close(db); return head;
}

/* --- Liste Yönetimi --- */
std* insertSorted(std *head, std *newS) {
    if (!head || newS->gpa >= head->gpa) { newS->next = head; return newS; }
    std *curr = head;
    while (curr->next && curr->next->gpa > newS->gpa) curr = curr->next;
    newS->next = curr->next; curr->next = newS;
    return head;
}

std* deleteStudent(std *head, int n) {
    std *curr = head, *prev = NULL;
    while (curr && curr->no != n) { prev = curr; curr = curr->next; }
    if (!curr) return head;
    if (!prev) head = curr->next; else prev->next = curr->next;
    course *c = curr->courses;
    while(c) { course *t = c->nextCourse; free(c); c = t; }
    free(curr); return head;
}

/* --- Ana Menü --- */
int main() {
    initDB(); std *head = loadDB(); int sec, n;
    while (1) {
        printf("\n1.Ekle 2.Listele 3.Sil 4.Kariyer Analizi 5.Basari Grafigi 6.CSV Aktar 7.Cikis\nSecim: ");
        if (scanf("%d", &sec) != 1) { while (getchar() != '\n'); continue; }
        if (sec == 1) {
            std *ns = (std*)malloc(sizeof(std));
            printf("Ad Soyad: "); scanf("%s %s", ns->firstName, ns->lastName);
            stringToUpper(ns->firstName); stringToUpper(ns->lastName);
            printf("No: "); scanf("%d", &ns->no);
            int cC; printf("Ders Sayisi: "); scanf("%d", &cC); ns->courses = NULL;
            int i;
            for(i=0; i<cC; i++) {
                course *nc = (course*)malloc(sizeof(course));
                printf("Ders Adi (OR: MATEMATIK): "); scanf("%s", nc->courseName); stringToUpper(nc->courseName);
                nc->yazili1 = getValidGrade("1.Y"); nc->yazili2 = getValidGrade("2.Y"); nc->sozlu = getValidGrade("S");
                aiCourseAnalyzer(nc); nc->nextCourse = ns->courses; ns->courses = nc;
            }
            aiStudentStatusAnalyzer(ns); head = insertSorted(head, ns);
        } else if (sec == 2) {
            std *t = head; while(t) { printf("\n%d: %s %s | GPA: %.2f | Rozet: %s", t->no, t->firstName, t->lastName, t->gpa, t->achievement_badge); t = t->next; }
        } else if (sec == 3) { printf("No: "); scanf("%d", &n); head = deleteStudent(head, n); }
        else if (sec == 4 || sec == 5) {
            printf("No: "); scanf("%d", &n); std *t = head; while(t && t->no != n) t = t->next;
            if(t && sec == 4) aiCareerAdvisor(t);
            else if(t && sec == 5) displayPerformanceGraph(t);
        } else if (sec == 6) {
            FILE *f = fopen("rapor.csv", "w");
            fprintf(f, "No;Ad;Soyad;GPA\n");
            std *t = head; while(t) { fprintf(f, "%d;%s;%s;%.2f\n", t->no, t->firstName, t->lastName, t->gpa); t = t->next; }
            fclose(f); printf("\n[+] CSV Olusturuldu.");
        } else { syncDB(head); break; }
    }
    return 0;
}
