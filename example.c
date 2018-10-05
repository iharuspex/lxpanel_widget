#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib/gi18n.h>

#include <string.h>

#include "panel.h"
#include "misc.h"
#include "plugin.h"

//Для сравнения строк
#define STREQV(a, b) (*(a) == *(b) && strcmp((a), (b)) == 0)

//Объявляем структуру нашего плагина
typedef struct {
    //Виджет, который будет содержать всебе иконку и менюшку
    GtkWidget *namew;
} my_plugin;

//Создаём объект
static my_plugin *mp;

//структура Plugin объявляется в src/plugin.h
//*mp_p - теперь класса Plugin
//Который как минимум задаёт родительский контейнер mp_p->pwid
//Наш виджет mp->namew в него "вставлен"
//А значок, к примеру, содержится в mp->namew
//Долго кощей смерть прятал
static Plugin *mp_p;

//Назавание говорит за себя
static char get_governor() {
    FILE *fpipe;
    const char governor[16];
    char line[16];

    //Проблема с распознованием вывода cpufreq-info
    //Скрипт /usr/local/bin/cpufreq-gov возвращает название текущего режима
    fpipe = (FILE*)popen("/usr/local/bin/cpufreq-gov","r");
    fgets(line, sizeof(line), fpipe);
    pclose(fpipe);

    return line;
}

//Назавание говорит за себя
static char get_frequency() {
    FILE *fpipe;
    const char governor[16];
    char line[16];

    //Проблема с распознованием вывода cpufreq-info
    //Скрипт /usr/local/bin/cpufreq-gov возвращает название текущего режима
    fpipe = (FILE*)popen("cpufreq-info -f -m","r");
    fgets(line, sizeof(line), fpipe);
    pclose(fpipe);
    line[strlen(line)-1]='\0';

    return line;
}

//Сделаем функцию обновления иконки при смене режима
static void refresh_icon() {
    //Узнаём текущий режим
    const char governor[16];
    char line[16] = get_governor();
    sprintf(governor,"%s",line);

    //mp_p->pwid - родительский контейнер типа GTKWidget, котрый содержит в себе mp->namew
    //Вынимаем виджет mp->namew и меняем значёк
    gtk_container_remove(GTK_CONTAINER(mp_p->pwid), mp->namew);

    //Если режим "ondemand"
    if(STREQV(governor,"ondemand"))
    {
        //Один значёк
        mp->namew = gtk_image_new_from_file("/usr/share/lxpanel/images/cpufreq_ond.png");
    }
    else
    {
        //Иначе другой
        mp->namew = gtk_image_new_from_file("/usr/share/lxpanel/images/cpufreq_perf.png");
    }
    //Вставляем виджет mp->namew обратно в родительский контейнер mp_p->pwid
    gtk_container_add(GTK_CONTAINER(mp_p->pwid), mp->namew);
    gtk_widget_show_all(mp_p->pwid);
}

//Функция смены режима
static void set_governor(GtkWidget *widget, char *p) {
    const char exec[32];
    sprintf(exec, "cpufreq-set -g %s", p);
    system(exec);
    refresh_icon();
}

//Создаём меню
static GtkWidget *mp_menu() {
    GtkMenu *menu = gtk_menu_new();
    GSList *group = NULL;
    GtkWidget *menuitem, *radio1, *radio2;

    FILE *fpipe;
    const char governor[16];
    const char frequency[16];
    char line[16];

    line = get_governor();
    sprintf(governor,"%s",line);

    line = get_frequency();
    sprintf(frequency,"%s",line);

    //1) Добавляем в меню текущую частоту, делаем пункт неактивным
    menuitem = gtk_menu_item_new_with_label(frequency);
    gtk_menu_append (menu, menuitem);
    gtk_widget_set_sensitive(menuitem, FALSE);
    gtk_widget_show (menuitem);

    //2) Добавляем разделитель
    menuitem = gtk_separator_menu_item_new();
    gtk_menu_append (menu, menuitem);
    gtk_widget_show (menuitem);

    //3) 2 радио-кнопки для переключением между режимами
    radio1 = gtk_radio_menu_item_new_with_label (group, "Экономия");
    group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (radio1));
    if(STREQV(governor,"ondemand"))
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (radio1), TRUE);
    }
    gtk_menu_append (menu, radio1);
    gtk_widget_show (radio1);

    //4) Вторая кнопка
    radio2 = gtk_radio_menu_item_new_with_label (group, "Производительность");
    if(STREQV(governor,"performance"))
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (radio2), TRUE);
    }
    gtk_menu_append (menu, radio2);
    gtk_widget_show (radio2);

    //При нажатии по радиокнопкам происходит переключение режимов
    g_signal_connect(G_OBJECT(radio1), "toggled", G_CALLBACK(set_governor), "ondemand");
    g_signal_connect(G_OBJECT(radio2), "toggled", G_CALLBACK(set_governor), "performance");

    return menu;
}

//Обработчик события нажатия по иконке плагина
static  gboolean clicked( GtkWidget *widget, GdkEventButton* evt, Plugin* plugin) {
    gtk_menu_popup(mp_menu(), NULL, NULL, NULL, NULL, evt->button, evt->time);
    return TRUE;
}

//Собственно конструктор объекта
static int constructor(Plugin *p) {
    //Cоздаём объект
    mechanism_mp = g_new0(my_plugin, 1);
    p->priv = mechanism_mp;

    //Делаем родительский виджет чувствительным (следим за событиями)
    //И убираем с него оформление окна
    p->pwid = gtk_event_box_new();
    GTK_WIDGET_SET_FLAGS(p->pwid, GTK_NO_WINDOW );
    gtk_container_set_border_width( GTK_CONTAINER(p->pwid), 0 );

    //Устанавливаем первоначальный значёк
    refresh_icon();

    //При нажатии срабатывает функция-обработчик clicked()
    g_signal_connect (G_OBJECT (p->pwid), "button_press_event", G_CALLBACK (clicked), (gpointer) p);

    gtk_widget_show(mechanism_mp->namew);
}

//Деструктор
static void destructor(Plugin *p) {
    my_plugin *mechanism_mp = (my_plugin *)p->priv;
    g_free(mechanism_mp);
}

//А здесь указываем информацию о нашем плагине
PluginClass mp_plugin_class = {
        PLUGINCLASS_VERSIONING,

        type : "my_plugin",
        name : N_("My super plugin"),
        version: "1.0",
        description : N_("Habrahabr"),

        constructor : constructor,
        destructor  : destructor,
        //В нашем плагине не используются,
        //но можно назначить обработчики для конфигурации, сохранения конфигурации
        //И перерисовки виджета, при изменении конфигурации
        config : NULL,
        save : NULL,
        panel_configuration_changed : NULL
};