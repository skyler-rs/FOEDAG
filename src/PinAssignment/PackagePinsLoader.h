/*
Copyright 2022 The Foedag team

GPL License

Copyright (c) 2022 The Open-Source FPGA Foundation

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "PackagePinsModel.h"

namespace FOEDAG {

class PackagePinsLoader : public QObject {
 public:
  PackagePinsLoader(PackagePinsModel *model, QObject *parent = nullptr);
  virtual ~PackagePinsLoader();
  virtual std::pair<bool, QString> load(const QString &fileName);
  virtual std::pair<bool, QString> loadHeader(const QString &fileName);

  void setModel(PackagePinsModel *model);

 protected:
  std::pair<bool, QString> getFileContent(const QString &fileName) const;
  void parseHeader(const QString &header);

 protected:
  PackagePinsModel *m_model{nullptr};
};

}  // namespace FOEDAG
