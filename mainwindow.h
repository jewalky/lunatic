#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_verticalSlider_valueChanged(int value);

    void on_comboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
