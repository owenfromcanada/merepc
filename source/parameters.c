/***********************************************************************************************************************
Copyright 2026 Owen Tosh.

This file is part of MerePC.

MerePC is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

MerePC is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with MerePC. If not, see
<https://www.gnu.org/licenses/>. 
***********************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "parameters.h"


#define MAX_LINE_LEN    300
#define FILENAME_LEN    100


static const char DATA_DIR[] = "./.merepcdata";
static const char PARAM_DELIM[] = ":";
static const char ARRAY_DELIM[] = ";";


void InitParameters(void) {
    struct stat st;
    if (stat(DATA_DIR, &st) == -1) {
        mkdir(DATA_DIR, 0700);
    }
}

void LoadParameters(const char * name, int parametercount, ParameterType * parameters) {
    char filename[FILENAME_LEN];
    char linebuf[MAX_LINE_LEN];

    sprintf(filename, "%s/%s", DATA_DIR, name);
    FILE * fp = fopen(filename, "r");
    if (fp == NULL) {
        return;
    }

    while (!feof(fp)) {
        // read line
        for (int lineidx = 0; lineidx < MAX_LINE_LEN - 1; lineidx++) {
            int c = fgetc(fp);
            if (c == '\n' || c == EOF) {
                linebuf[lineidx] = 0;
                break;
            }
            linebuf[lineidx] = (char)c;
        }

        // read line elements
        char * key = strtok(linebuf, PARAM_DELIM);
        if (key == NULL) {
            continue;
        }

        char * typestr = strtok(NULL, PARAM_DELIM);
        if (typestr == NULL) {
            continue;
        }
        int type;
        int res = sscanf(typestr, "%d", &type);
        if (res == 0) {
            continue;
        }

        char * countstr = strtok(NULL, PARAM_DELIM);
        if (countstr == NULL) {
            continue;
        }
        int count;
        res = sscanf(countstr, "%d", &count);
        if (res == 0) {
            continue;
        }

        char * value = strtok(NULL, PARAM_DELIM);
        if (value == NULL) {
            continue;
        }

        // find matching parameter
        for (int pidx = 0; pidx < parametercount; pidx++) {
            ParameterType * parameter = &parameters[pidx];

            if (strcmp(key, parameter->key) == 0) {
                if (type == parameter->type && count == parameter->count) {

                    // read parameter values
                    for (int k = 0; k < parameter->count; k++) {
                        char * element = strtok((k == 0 ? value : NULL), ARRAY_DELIM);
                        if (element == NULL) {
                            break;
                        }

                        if (parameter->type == TYPE_INT) {
                            sscanf(element, "%d", &(((int *)parameter->data)[k]));
                        }
                        else if (parameter->type == TYPE_FLOAT) {
                            sscanf(element, "%f", &(((float *)parameter->data)[k]));
                        }
                        else if (parameter->type == TYPE_STRING) {
                            sscanf(element, "%s", ((char **)parameter->data)[k]);
                        }
                    }

                }
                break;
            }
        }
    }

    fclose(fp);
}

void SaveParameters(const char * name, int parametercount, ParameterType * parameters) {
    char filename[FILENAME_LEN];

    sprintf(filename, "%s/%s", DATA_DIR, name);
    FILE * fp = fopen(filename, "w");
    if (fp == NULL) {
        return;
    }

    for (int pidx = 0; pidx < parametercount; pidx++) {
        ParameterType * parameter = &parameters[pidx];

        fprintf(fp, "%s%s", parameter->key, PARAM_DELIM);
        fprintf(fp, "%d%s", parameter->type, PARAM_DELIM);
        fprintf(fp, "%d%s", parameter->count, PARAM_DELIM);

        for (int k = 0; k < parameter->count; k++) {
            if (parameter->type == TYPE_INT) {
                fprintf(fp, "%d", ((int *)parameter->data)[k]);
            }
            else if (parameter->type == TYPE_FLOAT) {
                fprintf(fp, "%f", ((float *)parameter->data)[k]);
            }
            else if (parameter->type == TYPE_STRING) {
                fprintf(fp, "%s", ((char **)parameter->data)[k]);
            }

            fprintf(fp, "%s", ARRAY_DELIM);
        }

        fprintf(fp, "%s\n", PARAM_DELIM);
    }

    fclose(fp);
}
