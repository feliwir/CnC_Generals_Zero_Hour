/*
**	Command & Conquer Renegade(tm)
**	Copyright 2025 Electronic Arts Inc.
**  Copyright 2026 Stephan Vedder
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once
#include <QApplication>

class WDumpApp : public QApplication
{
    Q_OBJECT
public:
    WDumpApp(int &argc, char **argv);

    bool event(QEvent *event) override;

    bool DumpTextures; // output texture usage to stdout
    bool NoWindow; // don't open window
    QString Filename;
    FILE * TextureDumpFile;
};