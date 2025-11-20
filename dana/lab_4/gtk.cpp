#include <gtk/gtk.h>
#include <vector>
#include <string>
#include <iostream>
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

struct ListWidgetItem {
    std::string text_1, text_2, text_3, text_4, text_5;
    static unsigned int size;
};

struct MainWindow {
    GtkWidget *window;
    GtkWidget *main_box;
    GtkWidget *buttons_widget;
    GtkWidget *list_widget;
    GtkWidget *save_to_db;
    GtkWidget *information;
    GtkWidget *exit_btn;
    GtkWidget *check_box;
    
    GtkWidget *list_container;
};

unsigned int ListWidgetItem::size = 0;
bool is_hide = false;
std::vector<GtkWidget*> list_elements;

void exit_slot(GtkWidget *widget, gpointer data);
void information_slot(GtkWidget *widget, gpointer data);
void check_box_slot(GtkWidget *widget, gpointer data);
void add_item_to_list(GtkWidget *parent, ListWidgetItem list_data);
void clear_list(GtkWidget *parent);
void set_font_size(GtkWidget *widget, size_t font_size);

MainWindow* create_main_window() {
    MainWindow *mw = new MainWindow();
    
    mw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(mw->window), "lab04 by GTK+3");
    gtk_window_set_default_size(GTK_WINDOW(mw->window), 1600, 1000);
    gtk_window_set_resizable(GTK_WINDOW(mw->window), FALSE);
    
    mw->main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_container_add(GTK_CONTAINER(mw->window), mw->main_box);
    
    mw->buttons_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(mw->buttons_widget, 400, 990);
    gtk_widget_set_name(mw->buttons_widget, "buttons_widget");
    
    mw->list_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_size_request(mw->list_widget, 1185, 990);
    gtk_widget_set_name(mw->list_widget, "list_widget");
    
    mw->check_box = gtk_check_button_new_with_label("Скрытые");
    mw->information = gtk_button_new_with_label("О программе");
    mw->exit_btn = gtk_button_new_with_label("Выход");
    
    set_font_size(mw->check_box, 15);
    set_font_size(mw->information, 15);
    set_font_size(mw->exit_btn, 15);
    
    gtk_box_pack_start(GTK_BOX(mw->buttons_widget), mw->check_box, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(mw->buttons_widget), mw->information, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(mw->buttons_widget), mw->exit_btn, FALSE, FALSE, 5);
    
    gtk_box_pack_start(GTK_BOX(mw->main_box), mw->buttons_widget, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(mw->main_box), mw->list_widget, TRUE, TRUE, 5);
    
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    mw->list_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(scrolled_window), mw->list_container);
    gtk_box_pack_start(GTK_BOX(mw->list_widget), scrolled_window, TRUE, TRUE, 0);
    
    g_signal_connect(mw->exit_btn, "clicked", G_CALLBACK(exit_slot), NULL);
    g_signal_connect(mw->information, "clicked", G_CALLBACK(information_slot), NULL);
    g_signal_connect(mw->check_box, "toggled", G_CALLBACK(check_box_slot), mw);
    
    return mw;
}

void exit_slot(GtkWidget *widget, gpointer data) {
    gtk_main_quit();
}

void information_slot(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "Лабораторная работа 4\n---> Разработка кроссплатформенных программ\n---> Выполнилa: Карабаева Д.К\n---> Группа: 601-21");
    gtk_window_set_title(GTK_WINDOW(dialog), "О программе");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void check_box_slot(GtkWidget *widget, gpointer data) {
    MainWindow *mw = static_cast<MainWindow*>(data);
    gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
    
    clear_list(mw->list_container);
    
    add_item_to_list(mw->list_container, {"ФИО", "Название предмета", "Кол-во лекций", "Кол-во практик", "Кол-во лабораторных"});
    
    std::vector<table_element> table;
    
    if (state) {
        table = get_visible_subjects_table();
    } else {
        table = get_hidden_subjects_table();
    }
    
    for (auto i : table) {
        add_item_to_list(mw->list_container, { i.teacher_name,
                                            i.subject_name,
                                            i.lectures,
                                            i.practice,
                                            i.laboratory });
    }
}

void add_item_to_list(GtkWidget *parent, ListWidgetItem list_data) {
    GtkWidget *item = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_size_request(item, 1185, 50);
    
    GtkWidget *text_1 = gtk_label_new(list_data.text_1.c_str());
    GtkWidget *text_2 = gtk_label_new(list_data.text_2.c_str());
    GtkWidget *text_3 = gtk_label_new(list_data.text_3.c_str());
    GtkWidget *text_4 = gtk_label_new(list_data.text_4.c_str());
    GtkWidget *text_5 = gtk_label_new(list_data.text_5.c_str());
    
    gtk_label_set_xalign(GTK_LABEL(text_1), 0.5);
    gtk_label_set_xalign(GTK_LABEL(text_2), 0.0);
    gtk_label_set_xalign(GTK_LABEL(text_3), 0.5);
    gtk_label_set_xalign(GTK_LABEL(text_4), 0.5);
    gtk_label_set_xalign(GTK_LABEL(text_5), 0.5);
    
    gtk_widget_set_size_request(text_1, 237, 50);
    gtk_widget_set_size_request(text_2, 357, 50);
    gtk_widget_set_size_request(text_3, 197, 50);
    gtk_widget_set_size_request(text_4, 197, 50);
    gtk_widget_set_size_request(text_5, 197, 50);
    
    set_font_size(text_1, 15);
    set_font_size(text_2, 15);
    set_font_size(text_3, 15);
    set_font_size(text_4, 15);
    set_font_size(text_5, 15);
    
    gtk_box_pack_start(GTK_BOX(item), text_1, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item), text_2, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item), text_3, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item), text_4, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(item), text_5, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(parent), item, FALSE, FALSE, 0);
    
    ListWidgetItem::size += 1;
    list_elements.push_back(item);
    
    gtk_widget_show_all(item);
}

void clear_list(GtkWidget *parent) {
    for (GtkWidget *item : list_elements) {
        gtk_widget_destroy(item);
    }
    
    list_elements.clear();
    ListWidgetItem::size = 0;
}

void set_font_size(GtkWidget *widget, size_t font_size) {
    std::string css_class = "font-size-" + std::to_string(font_size);
    std::string css = "." + css_class + " { font-size: " + std::to_string(font_size) + "px; }";
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, css.c_str(), -1, NULL);
    
    GtkStyleContext *context = gtk_widget_get_style_context(widget);
    gtk_style_context_add_provider(context, 
                                  GTK_STYLE_PROVIDER(provider),
                                  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    gtk_style_context_add_class(context, css_class.c_str());
    
    g_object_unref(provider);
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

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);
    
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        "#buttons_widget, #list_widget {"
        "   background-color: grey;"
        "}", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                             GTK_STYLE_PROVIDER(provider),
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    
    MainWindow *main_window = create_main_window();
    
    add_item_to_list(main_window->list_container, {"ФИО", "Название предмета", "Кол-во лекций", "Кол-во практик", "Кол-во лабораторных"});
    
    std::vector<table_element> table = get_hidden_subjects_table();
    for (auto i : table) {
        add_item_to_list(main_window->list_container, { i.teacher_name,
                                                     i.subject_name,
                                                     i.lectures,
                                                     i.practice,
                                                     i.laboratory });
    }
    
    g_signal_connect(main_window->window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    gtk_widget_show_all(main_window->window);
    
    gtk_main();
    
    delete main_window;
    return 0;
}