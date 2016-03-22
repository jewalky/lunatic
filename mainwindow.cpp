#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "lunaticstyle.h"

#include <QStyleFactory>
#include <QWindow>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    LunaticStyle* style = new LunaticStyle();
    //StringList
    QVector<LunaticSubStyle>& subst = style->getSubStyles();
    ui->comboBox->clear();
    ui->comboBox->addItem("<Win9X>");
    for(int i = 0; i < subst.size(); i++)
        ui->comboBox->addItem(subst[i].ReadableName);

    QApplication::setStyle(new LunaticStyle());

    ui->mainToolBar->addAction("test action 1");
    ui->mainToolBar->addAction("test action 2");
    ui->toolBar->addAction("test action -1");
    ui->toolBar->addAction("test action -2");

    QAction* toggle_a = new QAction("toggle_action", ui->toolBar);
    toggle_a->setCheckable(true);
    ui->toolBar->addAction(toggle_a);

    ui->statusBar->addWidget(new QLabel("hihihi"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    ui->checkBox->setEnabled(!ui->checkBox->isEnabled());
    ui->checkBox_2->setEnabled(!ui->checkBox_2->isEnabled());
    ui->radioButton->setEnabled(!ui->radioButton->isEnabled());
    ui->pushButton_3->setEnabled(!ui->pushButton_3->isEnabled());
    ui->groupBox->setEnabled(!ui->groupBox->isEnabled());
    ui->progressBar->setEnabled(!ui->progressBar->isEnabled());
    ui->progressBar_2->setEnabled(!ui->progressBar_2->isEnabled());
    ui->spinBox->setEnabled(!ui->spinBox->isEnabled());
    ui->horizontalSlider->setEnabled(!ui->horizontalSlider->isEnabled());
    ui->verticalSlider->setEnabled(!ui->verticalSlider->isEnabled());
    ui->comboBox->setEnabled(!ui->comboBox->isEnabled());
    ui->listWidget->setEnabled(!ui->listWidget->isEnabled());
    ui->tableWidget->setEnabled(!ui->tableWidget->isEnabled());
    ui->tabWidget->setEnabled(!ui->tabWidget->isEnabled());
}

void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    ui->progressBar->setValue(value);
}

void MainWindow::on_verticalSlider_valueChanged(int value)
{
    ui->progressBar_2->setValue(value);
}

void MainWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    LunaticStyle* s = new LunaticStyle();

    QVector<LunaticSubStyle>& subst = s->getSubStyles();
    bool sw = false;
    for(int i = 0; i < subst.size(); i++)
    {
        if(subst[i].ReadableName == arg1)
        {
            s->switchSubStyle(subst[i]);
            sw = true;
            break;
        }
    }

    if(!sw) s->resetSubStyle();
    QApplication::setStyle(s);
}
