#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSet>
namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    void simplify_grammer(QString text);
    void erase_LF(QList<QString> x, QList<QString> y);
    void erase_LR(QList<QString> x, QList<QString> y);            //消除左递归
    QList<QString> get_first(QList<QString> x, QList<QString> y);                   //求first集合
    QList<QSet<QChar>> get_follow();
    QSet<QChar> getfirst(QString s);
    QString dfs(QList<QString> x, QList<QString> y, QString v, QString first, int &flag);
    void left_translation();
    QString left_tr_dfs(QChar ch, int &flag, int &text_count);

    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();
    void on_pushButton_simplify_grammer();
    void on_pushButton_erase_LF();
    void on_pushButton_erase_LR();
    void on_pushButton_get_first_follow();
    void on_pushButton_left_translation();

private:
    Ui::MainWindow *ui;
    QList<QString> final_x;
    QList<QString> final_y;
    QByteArray ary;
    int ascii;
};

#endif // MAINWINDOW_H
