#include "wdumpwin.h"
#include "ui_wdumpwin.h"
#include <QFileDialog>
#include <QItemSelectionModel>

WDumpWindow::WDumpWindow(QWidget *parent) : QMainWindow(parent)
{
    m_ui = new Ui::WDumpWindow();
    m_ui->setupUi(this);

    connect(m_ui->actionOpen, &QAction::triggered, this, &WDumpWindow::onOpen);
    connect(m_ui->actionExit, &QAction::triggered, this, &WDumpWindow::onExit);
}

WDumpWindow::~WDumpWindow()
{
    delete m_ui;
}

void WDumpWindow::onOpen()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open File"), "",
                                                    tr("W3D Files (*.w3d, *.W3D);;All Files (*)"));
    if (!fileName.isEmpty())
    {
        if(!m_chunkData.Load(fileName.toStdString().c_str()))
        {
            fprintf(stderr,"Failed to load file %s\n", fileName.toStdString().c_str());
        }
        m_ui->treeView->setModel(new ChunkDataModel(&m_chunkData, this));
        if (m_ui->treeView->selectionModel() != nullptr)
        {
            connect(m_ui->treeView->selectionModel(),
                    &QItemSelectionModel::selectionChanged,
                    this,
                    &WDumpWindow::onTreeSelectionChanged);
        }
    }
}

void WDumpWindow::onExit()
{
    close();
}

void WDumpWindow::onTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(deselected);

    if (selected.indexes().isEmpty())
    {
        return;
    }

    // Get the selected ChunkItem from the model
    QModelIndex index = selected.indexes().first();
    ChunkItem *item = static_cast<ChunkItem *>(index.internalPointer());
    if (item == nullptr || item->Type == nullptr || item->Type->Callback == nullptr)
    {
        return;
    }
    ChunkModel *model = new ChunkModel(this);
    item->Type->Callback(item, model);
    m_ui->tableView->setModel(model);
}
