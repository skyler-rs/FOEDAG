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
#include "PackagePinsView.h"

#include <QCompleter>
#include <QHeaderView>
#include <QStringListModel>

#include "BufferedComboBox.h"

namespace FOEDAG {

constexpr int NameCol{0};
constexpr int AvailCol{1};
constexpr int PortsCol{2};
constexpr int ModeCol{3};

PackagePinsView::PackagePinsView(PinsBaseModel *model, QWidget *parent)
    : PinAssignmentBaseView(model, parent) {
  header()->resizeSections(QHeaderView::ResizeToContents);
  for (auto &h : model->packagePinModel()->header()) {
    headerItem()->setText(h.id, h.name);
    headerItem()->setToolTip(h.id, h.description);
  }

  QTreeWidgetItem *topLevelPackagePin = new QTreeWidgetItem(this);
  topLevelPackagePin->setText(0, "All Pins");
  const auto banks = model->packagePinModel()->pinData();
  for (const auto &b : banks) {
    QTreeWidgetItem *bank = new QTreeWidgetItem(topLevelPackagePin);
    bank->setText(NameCol, b.name);
    bank->setText(AvailCol, QString::number(b.pinData.count()));
    const auto pins = b.pinData;
    for (auto &p : pins) {
      uint col = PortsCol + 1;
      QTreeWidgetItem *pinItem = new QTreeWidgetItem(bank);
      insertData(p.data, PinName, NameCol, pinItem);
      insertData(p.data, RefClock, col++, pinItem);
      insertData(p.data, Bank, col++, pinItem);
      insertData(p.data, ALT, col++, pinItem);
      insertData(p.data, DebugMode, col++, pinItem);
      insertData(p.data, ScanMode, col++, pinItem);
      insertData(p.data, MbistMode, col++, pinItem);
      insertData(p.data, Type, col++, pinItem);
      insertData(p.data, Dir, col++, pinItem);
      insertData(p.data, Voltage, col++, pinItem);
      insertData(p.data, PowerPad, col++, pinItem);
      insertData(p.data, Discription, col++, pinItem);
      insertData(p.data, Voltage2, col++, pinItem);
      insertData(p.data, RefClock, col++, pinItem);

      auto combo = new BufferedComboBox{this};
      combo->setModel(model->portsModel()->listModel());
      combo->setAutoFillBackground(true);
      m_allCombo.append(combo);
      combo->setEditable(true);
      auto completer{new QCompleter{model->portsModel()->listModel()}};
      completer->setFilterMode(Qt::MatchContains);
      combo->setCompleter(completer);
      combo->setInsertPolicy(QComboBox::NoInsert);
      connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
              [=]() {
                ioPortsSelectionHasChanged(indexFromItem(pinItem, PortsCol));
              });
      setItemWidget(pinItem, PortsCol, combo);
      model->packagePinModel()->insert(pinItem->text(NameCol),
                                       indexFromItem(pinItem, NameCol));

      auto modeCombo = new QComboBox{this};
      modeCombo->setEnabled(modeCombo->count() > 0);
      connect(
          modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          [=]() { modeSelectionHasChanged(indexFromItem(pinItem, ModeCol)); });
      setItemWidget(pinItem, ModeCol, modeCombo);
    }
    expandItem(bank);
  }
  connect(model->packagePinModel(), &PackagePinsModel::itemHasChanged, this,
          &PackagePinsView::itemHasChanged);
  connect(model->packagePinModel(), &PackagePinsModel::modeHasChanged, this,
          &PackagePinsView::modeChanged);
  expandItem(topLevelPackagePin);
  setAlternatingRowColors(true);
  setColumnWidth(NameCol, 170);
  setColumnWidth(ModeCol, 180);
  resizeColumnToContents(PortsCol);
  header()->setSectionResizeMode(PortsCol, QHeaderView::Fixed);
}

void PackagePinsView::SetMode(const QString &pin, const QString &mode) {
  QModelIndex index{match(pin)};
  if (index.isValid()) {
    auto combo =
        qobject_cast<QComboBox *>(itemWidget(itemFromIndex(index), ModeCol));
    if (combo) combo->setCurrentIndex(combo->findData(mode, Qt::DisplayRole));
  }
}

void PackagePinsView::ioPortsSelectionHasChanged(const QModelIndex &index) {
  // update here Mode selection
  auto item = itemFromIndex(index);
  auto combo =
      item ? qobject_cast<BufferedComboBox *>(itemWidget(item, PortsCol))
           : nullptr;
  if (combo) {
    updateModeCombo(combo->currentText(), index);
  }

  if (m_blockUpdate) return;

  if (combo) {
    auto port = combo->currentText();
    removeDuplications(port, combo);

    auto pin = item->text(NameCol);
    m_model->update(port, pin);
    m_model->portsModel()->itemChange(port, pin);

    // unset previous selection
    auto prevPort = combo->previousText();
    m_model->portsModel()->itemChange(prevPort, QString());
    emit selectionHasChanged();
  }
}

void PackagePinsView::modeSelectionHasChanged(const QModelIndex &index) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<QComboBox *>(itemWidget(item, ModeCol));
    if (combo) {
      m_model->packagePinModel()->updateMode(item->text(NameCol),
                                             combo->currentText());
      emit selectionHasChanged();
    }
  }
}

void PackagePinsView::insertData(const QStringList &data, int index, int column,
                                 QTreeWidgetItem *item) {
  if (data.count() > index) item->setText(column, data.at(index));
}

void PackagePinsView::updateModeCombo(const QString &port,
                                      const QModelIndex &index) {
  auto modeIndex = model()->index(index.row(), ModeCol, index.parent());
  QComboBox *modeCombo{
      qobject_cast<QComboBox *>(itemWidget(itemFromIndex(modeIndex), ModeCol))};
  if (modeCombo) {
    if (port.isEmpty()) {
      modeCombo->setCurrentIndex(0);
      modeCombo->setEnabled(false);
    } else {
      modeCombo->setEnabled(true);
      auto ioPort = m_model->portsModel()->GetPort(port);
      const bool output = ioPort.dir == "Output";
      QAbstractItemModel *modeModel =
          output ? m_model->packagePinModel()->modeModelTx()
                 : m_model->packagePinModel()->modeModelRx();
      if (modeCombo->model() != modeModel) {
        modeCombo->setModel(modeModel);
      }
    }
  }
}

void PackagePinsView::itemHasChanged(const QModelIndex &index,
                                     const QString &pin) {
  auto item = itemFromIndex(index);
  if (item) {
    auto combo = qobject_cast<QComboBox *>(itemWidget(item, PortsCol));
    if (combo) {
      m_blockUpdate = true;
      const int index = combo->findData(pin, Qt::DisplayRole);
      combo->setCurrentIndex(index != -1 ? index : 0);
      m_blockUpdate = false;
    }
  }
}

void PackagePinsView::modeChanged(const QString &pin, const QString &mode) {
  SetMode(pin, mode);
}

}  // namespace FOEDAG
