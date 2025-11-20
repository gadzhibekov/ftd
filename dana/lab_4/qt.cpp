#include <QApplication>
#include <QString>
#include <QLabel>
#include <QFont>
#include <QObject>
#include <QApplication>
#include <QMessageBox>
#include <QLayout>
#include <QLayoutItem>
#include <QMainWindow>
#include <QPushButton>
#include <QWidget>
#include <QCheckBox>

#include <iostream>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <iomanip>

const char* insertDataSQL = 
                            "INSERT INTO subjects (teacher_name, subject_name, lectures, practice, laboratory, is_visible) VALUES "
                            "('Иванов А.С.', 'Математический анализ', 40, 20, 0, 1),"
                            "('Петрова М.И.', 'Программирование на C++', 30, 15, 30, 0),"
                            "('Сидоров В.П.', 'Базы данных', 35, 10, 25, 1),"
                            "('Козлова Е.Н.', 'Веб-технологии', 25, 20, 15, 1),"
                            "('Николаев Д.К.', 'Операционные системы', 40, 15, 20, 0),"
                            "('Федорова О.Л.', 'Компьютерные сети', 30, 10, 25, 1),"
                            "('Морозов С.В.', 'Алгоритмы и структуры данных', 45, 25, 10, 1),"
                            "('Волкова Т.М.', 'Искусственный интеллект', 35, 15, 20, 1),"
                            "('Лебедев А.А.', 'Мобильная разработка', 20, 20, 20, 0),"
                            "('Семенова И.Р.', 'Тестирование ПО', 25, 10, 30, 1),"
                            "('Павлов Н.Г.', 'Компьютерная графика', 30, 15, 25, 1),"
                            "('Алексеева Л.Д.', 'Машинное обучение', 40, 20, 15, 0),"
                            "('Кузнецов Р.С.', 'Кибербезопасность', 35, 15, 20, 1),"
                            "('Орлова В.К.', 'Разработка игр', 25, 15, 30, 1),"
                            "('Васильев М.П.', 'Архитектура ЭВМ', 45, 20, 10, 0),"
                            "('Тихонова Е.В.', 'Функциональное программирование', 30, 20, 15, 1);";

struct table_element
{
    std::string id;
    std::string teacher_name;
    std::string subject_name;
    std::string lectures;
    std::string practice;
    std::string laboratory;
    std::string is_visible;
};

std::vector<table_element> get_hidden_subjects_table();
std::vector<table_element> get_visible_subjects_table();

struct MainWindow : QMainWindow
{
    MainWindow(QWidget* parent);

private:
    QWidget*        central_widget;
    QWidget*        buttons_widget;
    QWidget*        list_widget;
    QPushButton*    save_to_db;
    QPushButton*    information;
    QPushButton*    exit;
    QCheckBox*      check_box;

    void            exit_slot();
    void            information_slot();
    void            check_box_slot(int state);
};

struct ListWidgetItem
{
    QString text_1, text_2, text_3, text_4, text_5;
    static unsigned int size;
};

void add_item_to_list(QWidget* parent, ListWidgetItem list_data);
void clear_list(QWidget* parent);
void set_font_size(QWidget* widget, size_t font_size);

unsigned int ListWidgetItem::size   = 0;
bool                        is_hide = false;

std::vector<QWidget *> list_elements;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    central_widget  = new QWidget(this);

    this->setCentralWidget(central_widget);
    this->setMinimumSize(1600, 1000);
    this->setMaximumSize(1600, 1000);

    #ifdef __linux__
    this->setWindowTitle("lab04 by Lnux (Qt)");
    #elif defined(__WIN32)
    this->setWindowTitle("lab04 by Windows (Qt)");
    #endif // __linux__

    buttons_widget = new QWidget(central_widget);
    buttons_widget->setStyleSheet("background-color: grey;");
    buttons_widget->setGeometry(5, 5, 400, 990);

    list_widget = new QWidget(central_widget);
    list_widget->setStyleSheet("background-color: grey;");
    list_widget->setGeometry(410, 5, 1185, 990);

    information = new QPushButton(buttons_widget);
    information->setGeometry(50, 60, 300, 50);
    information->setText("О программе");
    set_font_size(information, 15);

    exit = new QPushButton(buttons_widget);
    exit->setGeometry(50, 115, 300, 50);
    exit->setText("Выход");
    set_font_size(exit, 15);

    check_box = new QCheckBox(buttons_widget);
    check_box->setGeometry(50, 5, 300, 50);
    check_box->setText("Скрытые");
    set_font_size(check_box, 15);

    QObject::connect(information, &QPushButton::clicked, this, &MainWindow::information_slot);
    QObject::connect(exit, &QPushButton::clicked, this, &MainWindow::exit_slot);
    QObject::connect(check_box, &QCheckBox::stateChanged, this, &MainWindow::check_box_slot);

    add_item_to_list(list_widget, {"ФИО", "Название предмета", "Кол-во лекций", "Кол-во практик", "Кол-во лабораторных"});

    std::vector<table_element> table = get_hidden_subjects_table();

    for (auto i : table)
    {
        add_item_to_list(list_widget, { QString::fromStdString(i.teacher_name) 
                                    ,   QString::fromStdString(i.subject_name)
                                    ,   QString::fromStdString(i.lectures)
                                    ,   QString::fromStdString(i.practice)
                                    ,   QString::fromStdString(i.laboratory)});
    }
}

void MainWindow::exit_slot()
{
    QApplication::quit();
}

void MainWindow::information_slot()
{   
    QMessageBox::information(nullptr, "О программе", "Лабораторная работа 4\n---> Разработка кроссплатформенных программ\n---> Выполнила: Карабаева Д.К\n---> Группа: 601-21");
}

void MainWindow::check_box_slot(int state)
{
    clear_list(list_widget);
    add_item_to_list(list_widget, {"ФИО", "Название предмета", "Кол-во лекций", "Кол-во практик", "Кол-во лабораторных"});

    std::vector<table_element> table;

    if (state == Qt::Checked)   table = get_visible_subjects_table();
    else                        table = get_hidden_subjects_table();

    for (auto i : table)
    {
        add_item_to_list(list_widget, { QString::fromStdString(i.teacher_name) 
                                    ,   QString::fromStdString(i.subject_name)
                                    ,   QString::fromStdString(i.lectures)
                                    ,   QString::fromStdString(i.practice)
                                    ,   QString::fromStdString(i.laboratory)});
    }
}

void add_item_to_list(QWidget* parent, ListWidgetItem list_data)
{
    QWidget* item = new QWidget(parent);
    item->setGeometry(0, ListWidgetItem::size * 50, 1185, 50);
    
    parent->show();
    
    QLabel* text_1 = new QLabel(item);
    text_1->setText(list_data.text_1);
    text_1->setGeometry(0, 0, 237, 50);
    text_1->setAlignment(Qt::AlignCenter);
    set_font_size(text_1, 15);
    text_1->show();

    QLabel* text_2 = new QLabel(item);
    text_2->setText(list_data.text_2);
    text_2->setGeometry(237, 0, 357, 50);
    set_font_size(text_2, 15);
    text_2->show();

    QLabel* text_3 = new QLabel(item);
    text_3->setText(list_data.text_3);
    text_3->setGeometry(594, 0, 197, 50);
    set_font_size(text_3, 15);
    text_3->show();

    QLabel* text_4 = new QLabel(item);
    text_4->setText(list_data.text_4);
    text_4->setGeometry(791, 0, 197, 50);
    set_font_size(text_4, 15);
    text_4->show();

    QLabel* text_5 = new QLabel(item);
    text_5->setText(list_data.text_5);
    text_5->setGeometry(988, 0, 197, 50);
    set_font_size(text_5, 15);
    text_5->show();

    ListWidgetItem::size += 1;
    list_elements.push_back(item);
    
    item->show();
    item->update();
}

void clear_list(QWidget* parent)
{
    for (QWidget* item : list_elements) 
    {
        item->deleteLater();
    }

    list_elements.clear();
    
    ListWidgetItem::size = 0;
    
    parent->update();
}

void set_font_size(QWidget* widget, size_t font_size)
{
    QFont font = widget->font();
    font.setPointSize(static_cast<int>(font_size));
    widget->setFont(font);
}

std::vector<table_element> get_hidden_subjects_table() 
{
    std::vector<table_element> result;

    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_open("../data.db", &db);
    
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    const char* selectSQL = "SELECT id, teacher_name, subject_name, lectures, practice, laboratory, is_visible FROM subjects WHERE is_visible = 0;";
    
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) 
    {
        table_element element;
        element.id = std::to_string(sqlite3_column_int(stmt, 0));
        element.teacher_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        element.subject_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        element.lectures = std::to_string(sqlite3_column_int(stmt, 3));
        element.practice = std::to_string(sqlite3_column_int(stmt, 4));
        element.laboratory = std::to_string(sqlite3_column_int(stmt, 5));
        element.is_visible = std::to_string(sqlite3_column_int(stmt, 6));
        
        result.push_back(element);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return result;
}

std::vector<table_element> get_visible_subjects_table() 
{
    std::vector<table_element> result;

    sqlite3* db;
    sqlite3_stmt* stmt;
    int rc;

    rc = sqlite3_open("../data.db", &db);
    
    if (rc) 
    {
        std::cerr << "Error opening database: " << sqlite3_errmsg(db) << std::endl;
        return result;
    }

    const char* selectSQL = "SELECT * FROM subjects WHERE is_visible = 1;";
    
    rc = sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) 
    {
        std::cerr << "Failed to prepare query: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return result;
    }

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) 
    {
        table_element element;
        element.id = std::to_string(sqlite3_column_int(stmt, 0));
        element.teacher_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        element.subject_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        element.lectures = std::to_string(sqlite3_column_int(stmt, 3));
        element.practice = std::to_string(sqlite3_column_int(stmt, 4));
        element.laboratory = std::to_string(sqlite3_column_int(stmt, 5));
        element.is_visible = std::to_string(sqlite3_column_int(stmt, 6));
        
        if (element.is_visible == "1") 
        {
            result.push_back(element);
        }
    }

    if (rc != SQLITE_DONE) 
    {
        std::cerr << "Error reading data: " << sqlite3_errmsg(db) << std::endl;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return result;
}

int main(int argc, char** argv)
{
    QApplication app(argc, argv);
    MainWindow main_window(nullptr);

    main_window.show();

    return app.exec();
}