#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDebug>
#include <QStack>
#include <QString>
#include <vector>
#include <cstring>
#include <set>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->button_simplify, SIGNAL(clicked()), this, SLOT(on_pushButton_simplify_grammer()));
    connect(ui->pb_erase_LR, SIGNAL(clicked()), this, SLOT(on_pushButton_erase_LR()));
    connect(ui->pb_erase_LF, SIGNAL(clicked()), this, SLOT(on_pushButton_erase_LF()));
    connect(ui->pb_get_ff, SIGNAL(clicked()), this, SLOT(on_pushButton_get_first_follow()));
    connect(ui->pb_left_translation, SIGNAL(clicked()), this, SLOT(on_pushButton_left_translation()));
    QHeaderView* headerView = ui->first_follow->verticalHeader();
    headerView->setHidden(true);//隐藏tablewidget自带行号列
    ui->analysis_process->setColumnWidth(1,150);    //设置表头宽度
    for(int i=79; i>=60; i--)
        ary.append(i);
    ascii=0;
}

MainWindow::~MainWindow()
{
    delete ui;
}
// 打开文件
void MainWindow::on_pushButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this,QObject::tr("选择文法规则"),"",QObject::tr("TXT(*.txt)"));
    //每次载入之前要清空文本 否则会不停的追加
    this->ui->textEdit->clear();

    if(!fileName.isNull())
    {
        QFile file(fileName);
        if(!file.open(QFile::ReadOnly | QFile::Text))
        {
            QMessageBox::warning(this,QObject::tr("Error"), tr("read file error:&1").arg(file.errorString()));
            return;
        }

        QTextStream in(&file);

        QString text = in.readAll();
        qDebug()<<text; //测试文本是否读取到
        QApplication::setOverrideCursor(Qt::WaitCursor);
        this->ui->textEdit->setText(text);
        //从文本栏获取文本内容既可以通过选择文件翻译也能通过复制文本再点击翻译
        text = this->ui->textEdit->toPlainText();
        text = text.simplified();
        simplify_grammer(text);
        QApplication::restoreOverrideCursor();
        simplify_grammer(text);
        ui->textEdit_2->clear();
    }

}
// 保存文件
void MainWindow::on_pushButton_2_clicked()
{
    QFileDialog fileDialog;
    QString fileName = fileDialog.getSaveFileName(this,"Open File","","Text File(*.txt)");
    if(fileName == "")
    {
        return;
    }
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::warning(this,"error","open file failure!");
        return;
    }
    else
    {
        QTextStream textStream(&file);
        QString str = ui->textEdit->toPlainText();
        textStream<<str;
        QMessageBox::warning(this,"tip","Save File Success!");
        file.close();
    }
}

void MainWindow::simplify_grammer(QString text)
{
    QString::const_iterator cit = NULL;     //遍历QString
    QList<QString> x;
    QList<QString> y;
    int flag = 0;
    //获取左右两边的文法
    for(cit=text.cbegin(); cit<text.cend(); cit++)
    {
        if(*cit != ' ' && flag==0)
        {
            x.push_back(*cit);
            flag=1;
        }
        else
        {
            if(*cit=='-')
                cit = cit+2;
            // 拼接目标规则
             QString temp = "";
             QChar h0 = 0x00;
             for(;*cit != ' '; cit++)
             {
                 //删除后面的/0乱码部分
                 if(*cit==h0)
                 {
                     break;
                 }
                 temp += *cit;
             }
             y.push_back(temp);
             if(*cit == ' ')
                 flag=0;
        }
    }

    // 删除有害规则
    for(int i=0; i<x.size(); i++)
    {
        if((y[i].length()==1 && y[i][0] == x[i][0]) || y[i].length()==0)
        {
            y.removeAt(i);
            x.removeAt(i);
            i--;    //删除元素 后面的元素会向前补齐
        }
    }

    //找出右边经过起始点推导所有可到达的元素
    QSet<QChar> init_y;
    init_y.insert(x[0][0]);  //起始的S
    int size = -1;
    while(size != init_y.size()) // 不停的推导yi 和 yi+1
    {
        size = init_y.size();
        for(auto in_y:init_y)
        {
            for(int i=0; i<x.size(); i++)
            {
                //左推导
                if(x[i][0] == in_y)
                {
                    // 遍历右边的元素
                    for(auto temp:y[i])
                    {
                        if(temp >= 'A' && temp <= 'Z')
                            init_y.insert(temp); //init_y 添加该可到达的元素
                    }
                }
            }
        }
    }
    //删除不可到达的元素
    for(int i=0; i<x.size(); i++)
    {
        // 若左边存在的元素无法被推导出来则删除该条规则
        if(!init_y.contains(x[i][0]))
        {
            x.removeAt(i);
            y.removeAt(i);
        }
    }
    qDebug()<<x;
    qDebug()<<y;

    /*删除不可终止：不可终止就是推不出全是终结符的串*/
    //找出可以推导出终结符字符串的非终结符
    QSet<QChar> canReachEnd;
    for(int i=0; i<x.size(); i++)
    {
        QList<QString> tem_s = y[i].split('|');
        for(auto temp:tem_s)
        {
            bool existTerminal = false;     //是否存在终结符
            for(int j=0; j<temp.size(); j++)
            {
                if(temp[j]>='A' && temp[j]<='Z')
                {
                    existTerminal = true;   //存在
                    break;
                }
            }
            if(existTerminal==false)
                canReachEnd.insert(x[i][0]); // 存入时为QChar类型
        }
    }
    // 参照删除不可到达的规则重新推导一遍 若推导产生的不含existTernimal则为不可终止
    size = -1;
    while(size != canReachEnd.size())
    {
        size = canReachEnd.size();
        for(int i=0; i<x.size(); i++)
        {
            if(!canReachEnd.contains(x[i][0])) //若该规则左边不含有可直接推导出终结符
            {
                QList<QString> temp_s = y[i].split('|');
                for(auto temp : temp_s)
                {
                    bool existNonTerminal = false;  //是否含非终结符号
                    for(int j=0; j<temp.size(); j++)
                    {
                        //非终结符且不在可以推导出终结符串的终结符列表中
                        if(temp[j]>='A' && temp[j]<='Z' && !canReachEnd.contains(temp[j]))
                        {
                            existNonTerminal = true;
                            break;
                        }
                    }
                    if(existNonTerminal==false)
                    {
                        canReachEnd.insert(x[i][0]);
                    }
                }
            }
        }
    }
    QSet<QChar> cannotstop;
    //删除不可终止的元素规则及无用产生式
    for(int i=0; i<x.size(); i++)
    {
        // 若左边存在的元素无法被推导出来则删除该条规则
        if(!canReachEnd.contains(x[i][0]))
        {
            cannotstop.insert(x[i][0]);
            x.removeAt(i);
            y.removeAt(i);
        }

    }
    //继续删除无用产生式
    for(int i=0; i<x.size(); i++)
    {
        bool useful = false;
        QList<QString> temp_s = y[i].split('|');
        QString temp1 = "";
        for(int j=0; j<temp_s.size(); j++)
        {
            QString temp = temp_s[j];
            bool containNil = false; //是否含有不可终止符号
            for(int k=0; k<temp.size(); k++)
            {
                if(cannotstop.contains(temp[k]))
                {
                    containNil = true;
                    useful = true;      //使用到了该无用规则
                    break;
                }
            }
            if(containNil==false)
                temp1 += temp + "|";
        }
        if(temp1.size()>0 && useful==true)  //含有
            y.replace(i, temp1);
        else if(useful == true)
        {
            x.removeAt(i);
            y.removeAt(i);
        }
    }
    //将结果写入文本框
    QString result = "";
    for(int i=0; i<x.size(); i++)
    {
        result += x[i]+"->"+y[i] + "\n";
    }
    qDebug()<<result;
    ui->textEdit_2->setText(result);
    final_x = x;
    final_y = y;

}

//消除左递归
void MainWindow::erase_LR(QList<QString> x, QList<QString> y)
{
    /* 做法:
     * 1. 逐个非终结符进行处理
     * 2. 将处理干净的非终结符代入未解决的非终结符中，并消除干净
     * 3. 反复实施1，2直至所有非终结符处理完毕
    */
    QList<QString> extraX;
    QList<QString> extraY;
    for(int i=0; i<x.size(); i++)
    {
        //将存在的左递归非终结符入extraX，extraY存入转化为右递归的结果

        //代入再消除
        QList<QString> temp_s;
        for(int j=i; j<x.size(); j++)
        {
            QString str_y = y[j];
            for(int k=0; k<extraX.size(); k++)
            {
                if(str_y.contains(extraX[k]))
                {
                    QList<QString> str_y_list = str_y.split("|");
                    QString restore_str_y = "";
                    for(auto syl : str_y_list)
                    {

                        // 该串含有可替代的非终结符
                        if(syl.contains(extraX[k]) && syl.size()>1)
                        {
                            if(syl[1]!='\'')
                            {
                                //找出是哪个非终结符
                                int index=0;
                                for(index=0; index<x.size(); index++)
                                    if(extraX[k]==x[index])
                                        break;
                                QList<QString> str_y_l2 = y[index].split("|");
                                QString replacePart = "";
                                //一一添加
                                for(auto syl2:str_y_l2)
                                {
                                    replacePart += syl2+syl.right(syl.size()-1)+"|";
                                }
                                restore_str_y += replacePart;
                            }
                            else
                                restore_str_y += syl + "|";
                        }
                        else
                            restore_str_y += syl + "|";
                    }
                    y.replace(j, restore_str_y.left(restore_str_y.size()-1));
                }
            }

        }
        qDebug()<<x;
        qDebug()<<y;

        //消除直接左递归
        temp_s = y[i].split('|');

        QString temp1 = "";
        QString temp2 = "";
        int flag = 0;
        for(auto temp : temp_s)
            if(temp[0]==x[i][0])
            {
                flag=1;
                if(!extraX.contains(QString(ary.at(ascii))))        //chaneg
                    extraX.append(QString(ary.at(ascii)));
                temp1 += temp.right(temp.size()-1);
                temp1 += QString(ary.at(ascii)) +"|";
                temp2 += temp.right(temp.size()-1)+QString(ary.at(ascii)) +"|";//
            }
            else
                temp2 += temp+ x[i] + "|";
        //如果存在左递归 替换原y的规则
        if(flag == 1)
        {
            temp1 += "@";
            extraY.append(temp1);
            ascii++;
            temp2 = temp2.left(temp2.size()-1); //获取左边size-1个字符
            y.replace(i,temp2);
        }
    }
    for(int i=0; i<extraX.size(); i++)
    {
        x.append(extraX[i]);
        y.append(extraY[i]);
    }
    //将结果写入文本框
    QString result = "";
    for(int i=0; i<x.size(); i++)
    {
        result += x[i]+"->"+y[i] + "\n";
    }
    qDebug()<<result;
    ui->textEdit_2->setText(result);
    final_x = x;
    final_y = y;
}

// 求first集合
QList<QString> MainWindow::get_first(QList<QString> x, QList<QString> y)
{
    QList<QString> x_fitst, x_set;
    int size = x.size();
    qDebug()<<x;
    qDebug()<<y;
    for(int i=0; i<size; i++)
    {
        QList<QString> ep_production = y[i].split("|");
        QString first = "";

        for(auto ep: ep_production)
        {
            if((ep[0]<='z' && ep[0]>='a') || ep[0]=='@' && !first.contains(QString(ep[0])))
                first += QString(ep[0]);
            else
            {
                QString temp_first="";
                int flag = 0;
                int temp_size = ep.size();
                first += dfs(x,y,ep,temp_first,flag);
                while(flag==1 && --temp_size)
                {
                    flag=0;
                    first += dfs(x,y,ep.right(temp_size),temp_first,flag);
                }
                if(flag==1) //字符串可以推导出空
                    if(!first.contains("@"))
                        first += "@";

            }
        }
        x_set.append(first);
    }
    return x_set;
}

QString MainWindow::dfs(QList<QString> x, QList<QString> y, QString v, QString first, int &flag)
{
    if(v=="@")
        return "";
    int i = 0;
    for(i=0;  i<x.size(); i++)  //右递归式不需要进行递归
        if(v[0]==x[i][0])
            break;
    QList<QString> ep_production = y[i].split("|");
    //遍历产生式
    for(auto ep:ep_production)
    {
        if(ep[0]<='z'&&ep[0]>='a')
            first += QString(ep[0]);
        else if(ep[0]!='@') //非终结符继续搜索
            dfs(x, y, ep, first, flag);
        else if(ep[0]=='@') //含有空集的情况
            flag=1;
    }
    return first;
}

// 提取左公因子
void MainWindow::erase_LF(QList<QString> x, QList<QString> y)
{

    //存在间接左公因子存在的情况 左递归处理时只能处理部分 需要将左递归处理后的文法规则对应的非终结符替换
    QList<QString> temp_s;
    for(int i=0; i<x.size(); i++)
    {
        QList<QString> str_y_list = y[i].split("|");
        if(str_y_list.size()==1)
            continue;
        QString restore_str_y = "";
        for(auto s_l : str_y_list)
        {
            if(s_l[0]>='A' && s_l[0]<='Z')
            {
                //找出是哪个非终结符
                int index=0;
                for(index=0; index<x.size(); index++)
                    if(s_l[0]==x[index][0])
                        break;
                QList<QString> str_y_l2 = y[index].split("|");
                QString replacePart = "";
                //一一添加
                for(auto syl2:str_y_l2)
                {
                    if(syl2=="@")
                        replacePart += s_l.right(s_l.size()-1)+"|";
                    else
                        replacePart += syl2+s_l.right(s_l.size()-1)+"|";
                }
                restore_str_y += replacePart;
            }
            else  //空集不用替换
                restore_str_y += s_l + "|";
        }
        y.replace(i, restore_str_y.left(restore_str_y.size()-1));
    }

    QList<QString> first = get_first(x, y);
    QList<QString> extraX, extraY;      //存放提取左公因子后产生的新规则
    //查看first是否有交集
    for(int i=0; i<x.size();i++)
    {
        QList<QString> str_y_list = y[i].split("|");
        if(str_y_list.size()==1)
            continue;
        QSet<QChar> str_getSet;
        for(auto f : first[i])
            str_getSet.insert(f);
        int count=1;
        //若去重后容量变小了说明有重复
        if(str_getSet.size() < first[i].size())
        {

            int temp_size = str_y_list.size();
            for(int j=0; j<temp_size; j++)
            {
                QString temp_factor = "";
                //添加新的y
                temp_factor += str_y_list[j].right(str_y_list[j].size()-1) + "|";
                int flag[temp_size] = {0};
                for(int k=j+1; k<temp_size; k++)
                    if(str_y_list[j][0] == str_y_list[k][0])
                    {
                        temp_factor += str_y_list[k].right(str_y_list[k].size()-1) + "|";
                        flag[k]=1;//标记k
                    }

                temp_factor = temp_factor.left(temp_factor.size()-1);   //删去最右边的"|"
                QList<QString> temp_count = temp_factor.split("|");
                //拆分后含有空集 补充@字符
                if(temp_count.contains(""))
                    temp_factor+="@";       //默认在后面添加空集

                if(temp_count.size()>1) //超过一个说明该产生式是左公因子
                {
                    QString add_number = QString(ary.at(ascii)); //change
                    ascii++;
                    count++;
                    str_y_list.replace(j, QString(str_y_list[j][0]) + add_number);
                    QString replaceY = "";
                    for(int kk=0; kk<temp_size; kk++)
                        if(flag[kk]==0)
                            replaceY += str_y_list[kk] + "|";
                    y.replace(i, replaceY.left(replaceY.size()-1));
                    extraX.append(add_number);
                    extraY.append(temp_factor);
                    str_y_list = y[i].split("|");
                    temp_size = str_y_list.size();
                }
            }
        }

    }
    for(int i=0; i<extraX.size(); i++)
    {
        x.append(extraX[i]);
        y.append(extraY[i]);
    }
    //将结果写入文本框
    QString result = "";
    for(int i=0; i<x.size(); i++)
    {
        result += x[i]+"->"+y[i] + "\n";
    }
    ui->textEdit_2->setText(result);
    final_x = x;
    final_y = y;
}
//获取follow集
QList<QSet<QChar>> MainWindow::get_follow()
{
    QList<QSet<QChar>> follow;
    for(int i=0; i<final_x.size(); i++)
    {
        QSet<QChar> qset;
        follow.append(qset);
    }
    follow[0].insert('$');
    qDebug()<<"follow";
    qDebug()<<follow;
    bool flag = true;
    while(flag)
    {
        flag=false;
        for(int i=0; i<final_x.size(); i++)
        {
            QList<QString> strList = final_y[i].split("|");
            for(auto s : strList)
            {//QList去重
                for(int j=0; j<s.size(); j++)
                {
                    if(s[j]>='A' && s[j]<='Z')
                    {
                        //找到对应的规则
                        int index=0;
                        for(; index<final_x.size(); index++)
                            if(s[j] == final_x[index][0])
                                break;
                        int size2 = follow[index].size();
                        QSet<QChar> first = getfirst(s.mid(j+1));
                        int hasEpsilon = first.remove('@');

                        for (auto item = first.begin();item!=first.end();item++)
                            follow[index].insert(*item);
                        if(hasEpsilon==1)
                            for (auto item = follow[i].begin();item!=follow[i].end();item++)
                                follow[index].insert(*item);
                        if(size2 != follow[index].size())
                            flag = true;
                    }
                }
            }
        }
    }
    qDebug()<<"get_follow";
    qDebug()<<follow;
    return follow;
}

QSet<QChar> MainWindow::getfirst(QString s)
{
    QList<QString> list = s.split("|");
    QSet<QChar> first;
    for(QString tmp_s : list)
    {
        int i;
        for(i=0; i<tmp_s.size(); i++)
        {
            if(tmp_s[i] >= 'A' && tmp_s<='Z')
            {
                int index=0;
                for(; index<final_x.size(); index++)
                    if(tmp_s[i] == final_x[index][0])
                        break;
                QSet<QChar> new_first = getfirst(final_y[index]);
                int hasEpisilon = new_first.remove('@');

                for (auto item = new_first.begin();item!=new_first.end();item++)
                    first.insert(*item);
                if(hasEpisilon == 0)
                    break;
            }
            else
            {
                first.insert(tmp_s[i]);
                break;
            }
        }
        if(i == tmp_s.size())
            first.insert('@');
    }

    return first;
}

//最左推导
void MainWindow::left_translation()
{
    QString text = ui->lineEdit->text();    //获取文本框内容
    QList<QString> first = get_first(final_x, final_y); //根据first去匹配合适的推导规则
    QList<QString> matchProcess, matchRule;
    int i;
    //一条条规则去匹配直至符合条件
    for(i=0; i<final_x.size(); i++)
    {
        int success_match = 0;
        QList<QString> list = final_y[i].split("|");
        int count = 0;
        //匹配每条产生式
        for(auto l : list)
        {
            int text_count = 0;
            QList<QString> temp_matchProcess, temp_matchRule;
            QString match_text = "";
            int j=0;
            QString l_copy = l;
            for(j=0; j<l.size();j++)
            {

                if(l[j]==text[text_count])
                {
                    text_count++;
                    match_text += QString(l[j]);
                }
                else if(l[j]>='A' && l[j]<='Z')    //为非终结符
                {
                    int flag = 1;
                    QString temp_match = left_tr_dfs(l[j], flag, text_count);
                    match_text += temp_match;
                    qDebug()<<match_text;
                    if(flag != 1)//不匹配
                        break;
                    else
                    {
                        temp_matchRule.append(QString(l[j]));
                        if(temp_matchProcess.size()>=1)
                            temp_matchProcess.append(temp_matchProcess.back().mid(0,text_count-1)+temp_match+l.mid(j+1));
                        else
                            temp_matchProcess.append(l.mid(0,j)+temp_match+l.mid(j+1));
                    }
                }
                else
                    break;  //不匹配 跳出规则
            }
            if(match_text==text)
            {
                success_match=1;
                matchRule = temp_matchRule;
                matchProcess = temp_matchProcess;
                matchRule.prepend(final_x[i]);
                matchProcess.prepend(list[count]);
                break;
            }
            count++;
        }
        if (success_match==1)
            break;
    }
    qDebug()<<"判断开始";
    if(i!=final_x.size())
    {
        qDebug()<<"match success";
        qDebug()<<matchRule;
        qDebug()<<matchProcess;
        for(int i=0; i<matchRule.size(); i++)
        {
            int j=0;
            for(; j<final_x.size(); j++)
                if(matchRule[i]==final_x[j])
                    break;
            ui->analysis_process->setItem(i, 1, new QTableWidgetItem(final_x[j]+"->"+final_y[j]));
            ui->analysis_process->setItem(i, 0, new QTableWidgetItem(matchProcess[i]));
        }
    }
    else
        qDebug()<<"not match";
}

QString MainWindow::left_tr_dfs(QChar ch, int &flag, int &text_count)
{
    if(flag == 0)//不匹配
        return "";
    QString translation = "";
    QString text = ui->lineEdit->text();
    //查看是第几条规则
    int index = 0;
    for(; index<final_x.size(); index++)
        if(final_x[index][0]==ch)
            break;
    QList<QString> temp_list = final_y[index].split("|");
    for(auto t_l : temp_list)
    {
        int ii=0;
        for(ii=0; ii<t_l.size(); ii++)
        {
            if(t_l[ii]==text[text_count])
            {
                translation += t_l[ii];
                text_count++;
                flag=1;
            }
            else if(t_l[ii]>='A' && t_l[ii]<='Z')//进入循环
                left_tr_dfs(t_l[ii],flag, text_count);
            else
            {
                flag = 0;
                translation = "";//置空
                break;
            }
        }
        if(ii == t_l.size())   //匹配完毕
            return translation;
    }
    return translation;
}

// 点击触发化简功能
void MainWindow::on_pushButton_simplify_grammer()
{
    QString text1 = this->ui->textEdit->toPlainText();
    QString text2 = this->ui->textEdit_2->toPlainText();
    text1 = text1.simplified();
    text2 = text2.simplified();
    if(text2.size()>0)
        simplify_grammer(text2);
    else
        simplify_grammer(text1);
}

// 触发消除左递归函数
void MainWindow::on_pushButton_erase_LR()
{
    erase_LR(final_x, final_y);
}

//触发提取左公因子
void MainWindow::on_pushButton_erase_LF()
{
    erase_LF(final_x, final_y);
}
//触发最左推导
void MainWindow::on_pushButton_left_translation()
{
    ui->analysis_process->clearContents();
    left_translation();
}
// 触发获取first_follow集合
void MainWindow::on_pushButton_get_first_follow()
{
    ui->first_follow->clearContents();
    QList<QString> first= get_first(final_x, final_y);
    //去重
    QList<QString> x_set_set;
    for(int i=0; i<first.size(); i++)
    {
        QString temp1 = "";
        for(int j=0; j<first[i].size(); j++)
            if(!temp1.contains(QString(first[i][j])))
                temp1 += QString(first[i][j]);
        x_set_set.append(temp1);
    }
    QList<QSet<QChar>> follow = get_follow();
    //将QSet存入string中方便输出
    QList<QString> new_follow;
    for(int i=0; i<follow.size(); i++)
    {
        QString temp="";
        for(auto f:follow[i])
            temp += QString(f);
        new_follow.append(temp);
    }

    //输出到表格
    int count = 0;
    for(int i=0; i<x_set_set.size(); i++)
    {
        ui->first_follow->setItem(i, 0, new QTableWidgetItem(x_set_set[i]));
        ui->first_follow->setItem(i, 1, new QTableWidgetItem(new_follow[i]));
    }
}
