#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZONAS 5
#define DIAS 30
#define CONTAM 4  // 0 CO2, 1 SO2, 2 NO2, 3 PM2.5

typedef struct {
    char nombre[40];
    float historial[DIAS][CONTAM];   // 30 dias x 4 contaminantes
    float temp;
    float viento;
    float humedad;
} Zona;

const char *NOMBRES_CONTAM[CONTAM] = {"CO2", "SO2", "NO2", "PM2.5"};

/* Limites de referencia (ejemplo)
   Puedes ajustar estos valores segun la consigna o la normativa que te indiquen */
float limites[CONTAM] = {1000.0f, 20.0f, 40.0f, 15.0f};

void inicializar(Zona zonas[]) {
    for (int i = 0; i < ZONAS; i++) {
        sprintf(zonas[i].nombre, "Zona %d", i + 1);
        zonas[i].temp = 25.0f;
        zonas[i].viento = 2.0f;
        zonas[i].humedad = 50.0f;

        for (int d = 0; d < DIAS; d++) {
            for (int c = 0; c < CONTAM; c++) {
                zonas[i].historial[d][c] = 0.0f;
            }
        }
    }
}

int guardarDatos(const char *archivo, Zona zonas[]) {
    FILE *f = fopen(archivo, "wb");
    if (!f) return 0;

    size_t escritos = fwrite(zonas, sizeof(Zona), ZONAS, f);
    fclose(f);
    return escritos == ZONAS;
}

int cargarDatos(const char *archivo, Zona zonas[]) {
    FILE *f = fopen(archivo, "rb");
    if (!f) return 0;

    size_t leidos = fread(zonas, sizeof(Zona), ZONAS, f);
    fclose(f);
    return leidos == ZONAS;
}

void ingresarClima(Zona *z) {
    printf("Temperatura actual (C): ");
    scanf("%f", &z->temp);

    printf("Viento actual (m/s): ");
    scanf("%f", &z->viento);

    printf("Humedad actual (%%): ");
    scanf("%f", &z->humedad);
}

void ingresarHistorial(Zona *z) {
    printf("Ingreso de historial (30 dias) para %s\n", z->nombre);

    for (int d = 0; d < DIAS; d++) {
        printf("Dia %d\n", d + 1);
        for (int c = 0; c < CONTAM; c++) {
            printf("  %s: ", NOMBRES_CONTAM[c]);
            scanf("%f", &z->historial[d][c]);
        }
    }
}

void calcularPromedios30Dias(Zona zonas[], float promedios[ZONAS][CONTAM]) {
    for (int i = 0; i < ZONAS; i++) {
        for (int c = 0; c < CONTAM; c++) {
            float suma = 0.0f;
            for (int d = 0; d < DIAS; d++) suma += zonas[i].historial[d][c];
            promedios[i][c] = suma / DIAS;
        }
    }
}

/* Promedio ponderado de los ultimos 7 dias con ajuste simple por clima */
float predecirContaminante(Zona *z, int c) {
    int dias = 7;
    float sumaPesos = 0.0f;
    float suma = 0.0f;

    for (int k = 0; k < dias; k++) {
        int idx = DIAS - 1 - k;      /* dias recientes al final */
        float peso = (float)(dias - k); /* 7,6,5,... */
        suma += z->historial[idx][c] * peso;
        sumaPesos += peso;
    }

    float pred = suma / sumaPesos;

    /* Ajustes por clima */
    if (z->viento < 1.5f) pred *= 1.08f;       /* poco viento, se concentra */
    else if (z->viento > 4.0f) pred *= 0.95f;  /* buen viento, dispersa */

    if (c == 3 && z->humedad > 70.0f) pred *= 1.05f; /* PM2.5 con humedad alta */

    return pred;
}

void predecir24h(Zona zonas[], float pred[ZONAS][CONTAM]) {
    for (int i = 0; i < ZONAS; i++) {
        for (int c = 0; c < CONTAM; c++) {
            pred[i][c] = predecirContaminante(&zonas[i], c);
        }
    }
}

int nivelRiesgo(float valor, float limite) {
    if (valor <= limite) return 0;            /* Normal */
    if (valor <= limite * 1.2f) return 1;     /* Moderado */
    if (valor <= limite * 1.5f) return 2;     /* Alto */
    return 3;                                 /* Muy alto */
}

void imprimirAlerta(int nivel) {
    if (nivel == 0) printf("NORMAL");
    else if (nivel == 1) printf("MODERADO");
    else if (nivel == 2) printf("ALTO");
    else printf("MUY ALTO");
}

void generarRecomendaciones(int nivel) {
    if (nivel == 0) {
        printf("Sin acciones especiales, mantener monitoreo\n");
    } else if (nivel == 1) {
        printf("Reducir uso de vehiculos particulares y promover transporte publico\n");
        printf("Evitar ejercicio intenso al aire libre en horas pico\n");
    } else if (nivel == 2) {
        printf("Restringir trafico en zonas criticas y reforzar teletrabajo cuando sea posible\n");
        printf("Revisar emisiones de industrias y aplicar controles temporales\n");
        printf("Recomendar uso de mascarilla en grupos vulnerables\n");
    } else {
        printf("Activar protocolo de emergencia ambiental\n");
        printf("Suspender actividades al aire libre y cerrar temporalmente fuentes industriales contaminantes\n");
        printf("Habilitar atencion prioritaria en centros de salud y comunicacion masiva a la ciudadania\n");
    }
}

void exportarReporte(const char *archivo, Zona zonas[],
                     float prom[ZONAS][CONTAM], float pred[ZONAS][CONTAM]) {
    FILE *f = fopen(archivo, "w");
    if (!f) return;

    fprintf(f, "REPORTE DE CONTAMINACION Y PREDICCION 24H\n\n");

    for (int i = 0; i < ZONAS; i++) {
        fprintf(f, "Zona: %s\n", zonas[i].nombre);
        fprintf(f, "Clima actual: Temp %.2f C, Viento %.2f m/s, Humedad %.2f %%\n",
                zonas[i].temp, zonas[i].viento, zonas[i].humedad);

        for (int c = 0; c < CONTAM; c++) {
            int rProm = nivelRiesgo(prom[i][c], limites[c]);
            int rPred = nivelRiesgo(pred[i][c], limites[c]);

            fprintf(f, "  %s  Prom30d %.2f  Pred24h %.2f  Limite %.2f  RiesgoProm ",
                    NOMBRES_CONTAM[c], prom[i][c], pred[i][c], limites[c]);

            if (rProm == 0) fprintf(f, "NORMAL");
            if (rProm == 1) fprintf(f, "MODERADO");
            if (rProm == 2) fprintf(f, "ALTO");
            if (rProm == 3) fprintf(f, "MUY ALTO");

            fprintf(f, "  RiesgoPred ");
            if (rPred == 0) fprintf(f, "NORMAL");
            if (rPred == 1) fprintf(f, "MODERADO");
            if (rPred == 2) fprintf(f, "ALTO");
            if (rPred == 3) fprintf(f, "MUY ALTO");

            fprintf(f, "\n");
        }
        fprintf(f, "\n");
    }

    fclose(f);
}

int main() {
    Zona zonas[ZONAS];
    float promedios[ZONAS][CONTAM] = {0};
    float prediccion[ZONAS][CONTAM] = {0};

    const char *ARCH_DATOS = "data/datos_contaminacion.bin";
    const char *ARCH_REPORTE = "out/reporte_contaminacion.txt";

    inicializar(zonas);

    if (cargarDatos(ARCH_DATOS, zonas)) {
        printf("Datos cargados desde archivo\n");
    } else {
        printf("No existe archivo previo, se inicia con valores en cero\n");
    }

    int op;
    do {
        printf("\n--- MENU ---\n");
        printf("1. Ingresar clima por zona\n");
        printf("2. Ingresar historial 30 dias por zona\n");
        printf("3. Calcular promedios 30 dias\n");
        printf("4. Predecir 24 horas\n");
        printf("5. Mostrar alertas y recomendaciones\n");
        printf("6. Exportar reporte\n");
        printf("0. Guardar y salir\n");
        printf("Opcion: ");
        scanf("%d", &op);

        if (op == 1) {
            int z;
            printf("Zona (1-%d): ", ZONAS);
            scanf("%d", &z);
            if (z >= 1 && z <= ZONAS) ingresarClima(&zonas[z - 1]);
        } else if (op == 2) {
            int z;
            printf("Zona (1-%d): ", ZONAS);
            scanf("%d", &z);
            if (z >= 1 && z <= ZONAS) ingresarHistorial(&zonas[z - 1]);
        } else if (op == 3) {
            calcularPromedios30Dias(zonas, promedios);
            printf("Promedios calculados\n");
        } else if (op == 4) {
            predecir24h(zonas, prediccion);
            printf("Prediccion calculada\n");
        } else if (op == 5) {
            for (int i = 0; i < ZONAS; i++) {
                printf("\nZona: %s\n", zonas[i].nombre);
                for (int c = 0; c < CONTAM; c++) {
                    float val = prediccion[i][c];
                    int r = nivelRiesgo(val, limites[c]);
                    printf("  %s Pred24h %.2f Lim %.2f Riesgo ",
                           NOMBRES_CONTAM[c], val, limites[c]);
                    imprimirAlerta(r);
                    printf("\n");
                    generarRecomendaciones(r);
                }
            }
        } else if (op == 6) {
            exportarReporte(ARCH_REPORTE, zonas, promedios, prediccion);
            printf("Reporte exportado a %s\n", ARCH_REPORTE);
        }

    } while (op != 0);

    guardarDatos(ARCH_DATOS, zonas);
    printf("Datos guardados, fin\n");
    return 0;
}
