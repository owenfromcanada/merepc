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

#define TYPE_INT    0
#define TYPE_FLOAT  1
#define TYPE_STRING 2

typedef struct {
    char * key;
    int type;
    int count;
    void * data;
} ParameterType;

void InitParameters(void);
void LoadParameters(const char * name, int parametercount, ParameterType * parameters);
void SaveParameters(const char * name, int parametercount, ParameterType * parameters);
