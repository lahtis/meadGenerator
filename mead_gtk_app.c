#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Global Constants and Calculation Logic (Adapted from mead_calculator.c) ---

#define GRAVITY_POINTS_PER_UNIT 35.0
#define KG_TO_LBS 2.20462
#define L_TO_GAL 0.264172

// Global Widget Pointers to access UI elements in the callback function
GtkWidget *volume_entry;
GtkWidget *abv_entry;
GtkWidget *unit_combobox;
GtkWidget *sweetness_combobox;
GtkWidget *turbo_switch;

GtkWidget *og_label;
GtkWidget *fg_label;
GtkWidget *honey_label;
GtkWidget *water_label;
GtkWidget *message_label; // For error messages

// Pääikkuna tarvitaan dialogien ankkurointiin
GtkWidget *main_window;

// --- Info Dialog Content Definitions ---

const char *WATER_INFO =
    "<b>Veden laatu simanvalmistuksessa</b>\n\n"
    "Veden laatu on ratkaiseva käymisen onnistumiselle ja lopulliselle maulle. Se vaikuttaa hiivan toimintaan, suutuntumaan ja mausteiden tai hedelmien aromin irtoamiseen.\n\n"
    "<b>Tärkeimmät huomiot:</b>\n"
    "• <b>Kloori/Kloramiini:</b> Täytyy poistaa! Aiheuttavat epämiellyttäviä 'lääkemäisiä' sivumakuja. Käytä Campden-tabletteja tai hiilisuodatinta.\n"
    "• <b>Mineraalipitoisuus (Kovuus):</b> Kalsiumin ja magnesiumin kaltaiset mineraalit ovat hiivaravinteita. Täysin tislattu vesi voi vaatia mineraalilisäyksiä.\n"
    "• <b>pH:</b> Hiiva suosii hieman hapanta ympäristöä (pH 3.0–4.0). Korkea alkaliniteetti vesijohtovedessä voi stressata hiivaa.\n";

const char *HONEY_INFO =
    "<b>Tärkeimmät hunajalajikkeet siman valmistukseen</b>\n\n"
    "Hunajan kukkaislähde määrittää siman värin, aromin ja lopullisen maun.\n\n"
    "<b>Yleisimmät lajikkeet:</b>\n"
    "• <b>Puna-apila (Clover):</b> Vaalea, hienovarainen maku. Erinomainen perinteisiin simoihin. Yleisin ja helpoin saatavilla.\n"
    "• <b>Appelsiininkukka (Orange Blossom):</b> Sitruksinen, kukkainen tuoksu. Arvostettu kevyemmissä simoissa ja melomeleissä (hedelmäsimat).\n"
    "• <b>Niittyhunaja (Wildflower):</b> Erittäin vaihteleva, rikas ja monimutkainen. Sopii maustettuihin simoihin (Metheglins).\n"
    "• <b>Tattari (Buckwheat):</b> Erittäin tumma, rikas ja voimakas, usein melassimainen. Vaatii pitkää kypsytystä.\n";

/**
 * @brief Opens a new GTK dialog window to display information.
 * @param parent_window The main window to anchor the dialog to.
 * @param title The title of the dialog.
 * @param message The content of the dialog (can use Pango markup).
 */
void show_info_dialog(GtkWidget *parent_window, const char *title, const char *message) {
    GtkWidget *dialog, *content_area, *label;

    // Create a new dialog with OK button
    dialog = gtk_dialog_new_with_buttons(
        title,
        GTK_WINDOW(parent_window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "OK", GTK_RESPONSE_ACCEPT,
        NULL
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 300);

    // Get the content area and add the label
    content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), message);
    gtk_label_set_line_wrap(GTK_LABEL(label), TRUE); // Enable text wrapping
    gtk_label_set_xalign(GTK_LABEL(label), 0.0); // Align text to left
    gtk_container_set_border_width(GTK_CONTAINER(content_area), 10);

    gtk_container_add(GTK_CONTAINER(content_area), label);

    // Show the dialog and wait for response
    gtk_widget_show_all(dialog);
    gtk_dialog_run(GTK_DIALOG(dialog));

    // Destroy the dialog when closed
    gtk_widget_destroy(dialog);
}

/**
 * @brief Callback for the Water Info button.
 */
static void on_water_info_clicked(GtkWidget *widget, gpointer data) {
    show_info_dialog(main_window, "Veden laatu", WATER_INFO);
}

/**
 * @brief Callback for the Honey Info button.
 */
static void on_honey_info_clicked(GtkWidget *widget, gpointer data) {
    show_info_dialog(main_window, "Hunajalajikkeet", HONEY_INFO);
}


/**
 * @brief Calculates the required Original Gravity (OG) based on target ABV and sweetness.
 * @param abv Target Alcohol by Volume percentage.
 * @param sweetness String representing the desired sweetness level.
 * @param is_turbo Flag: 1 for Standard, 2 for Turbo Yeast.
 * @return double The required Original Gravity (e.g., 1.100), or 0.0 on error.
 */
double get_target_og(int abv, const char* sweetness, int is_turbo) {
    double fg;

    // Standard FG estimates for different sweetness levels (used only if not turbo)
    if (is_turbo == 1) {
        if (strcasecmp(sweetness, "Dry") == 0) {
            fg = 1.000;
        } else if (strcasecmp(sweetness, "Semi-Sweet") == 0) {
            fg = 1.010;
        } else if (strcasecmp(sweetness, "Sweet") == 0) {
            fg = 1.020;
        } else if (strcasecmp(sweetness, "Dessert") == 0) {
            fg = 1.030;
        } else {
            return 0.0; // Invalid sweetness
        }
    } else {
        // TURBO YEAST MODE: Assumes fermentation goes to bone dry
        fg = 1.000;
    }

    // Mead/Wine ABV formula approximation: ABV = (OG - FG) * 131.25
    // Rearranging for OG: OG = FG + (ABV / 131.25)
    double og = fg + ((double)abv / 131.25);

    // OG is represented as 1.XXX, so we round the result
    return round(og * 1000.0) / 1000.0;
}

/**
 * @brief Performs the core ingredient calculations based on the user inputs
 * and updates the global GTK labels.
 * @param volume_val Batch volume value.
 * @param abv_val Target ABV value.
 * @param unit_str Unit string ("Gallons" or "Liters").
 * @param sweetness_str Sweetness string ("Dry", "Sweet", etc.).
 * @param is_turbo_mode 1 for standard, 2 for turbo.
 */
void calculate_ingredients(double volume_val, int abv_val, const char* unit_str, const char* sweetness_str, int is_turbo_mode) {
    double target_og = get_target_og(abv_val, sweetness_str, is_turbo_mode);

    if (target_og == 0.0) {
        gtk_label_set_text(GTK_LABEL(message_label), "Virhe: Virheellinen makeustaso. Käytä Dry, Semi-Sweet, Sweet tai Dessert.");
        return;
    }

    // Sanity Check
    if (target_og > 1.225) {
        gtk_label_set_markup(GTK_LABEL(message_label), "<span foreground='orange'>VAROITUS: Laskettu OG (1.225+) on erittäin korkea. Kokeile pienempää ABV:tä.</span>");
        // Do not return, let the calculation continue but warn the user.
    } else {
        gtk_label_set_text(GTK_LABEL(message_label), ""); // Clear previous error
    }

    // --- Core Calculation ---
    double fg = is_turbo_mode == 1 ? (strcasecmp(sweetness_str, "Dry") == 0 ? 1.000 : strcasecmp(sweetness_str, "Semi-Sweet") == 0 ? 1.010 : strcasecmp(sweetness_str, "Sweet") == 0 ? 1.020 : 1.030) : 1.000;

    double honeyAmount;
    double waterAmount;
    const char* honeyUnit;
    const char* waterUnit;

    if (strcasecmp(unit_str, "Gallons") == 0) {
        double volume_gal = volume_val;
        double gravity_points_needed = (target_og - 1.000) * 1000.0 * volume_gal;
        double honey_lbs = gravity_points_needed / GRAVITY_POINTS_PER_UNIT;

        // Honey displacement: assume 0.65 gallons per 10 lbs.
        double honey_volume_gal = (honey_lbs / 10.0) * 0.65;
        double water_gal = volume_gal - honey_volume_gal;

        honeyAmount = honey_lbs;
        waterAmount = fmax(0.0, water_gal); // Use fmax for non-negative
        honeyUnit = "lbs";
        waterUnit = "gallons";

    } else { // Metric (Liters/Kg)
        double volume_L = volume_val;
        double volume_gal = volume_L * L_TO_GAL;

        double gravity_points_needed = (target_og - 1.000) * 1000.0 * volume_gal;
        double honey_lbs_calc = gravity_points_needed / GRAVITY_POINTS_PER_UNIT;

        // Convert honey from Lbs to Kilograms
        double honey_kg = honey_lbs_calc / KG_TO_LBS;

        // Honey displacement: assume 1 kg displaces ~0.74 Liters
        double honey_volume_L = honey_kg * 0.74;
        double water_L = volume_L - honey_volume_L;

        honeyAmount = honey_kg;
        waterAmount = fmax(0.0, water_L);
        honeyUnit = "kg";
        waterUnit = "liters";
    }

    // --- Update GTK Labels ---
    char buffer[100];

    // OG/FG Labels
    snprintf(buffer, sizeof(buffer), "OG (Ominaispaino): <b>%.3f</b>", target_og);
    gtk_label_set_markup(GTK_LABEL(og_label), buffer);
    snprintf(buffer, sizeof(buffer), "FG (Loppupaino): <b>%.3f</b>", fg);
    gtk_label_set_markup(GTK_LABEL(fg_label), buffer);

    // Honey Label
    snprintf(buffer, sizeof(buffer), "Tarvittava hunaja: <b>%.2f %s</b>", honeyAmount, honeyUnit);
    gtk_label_set_markup(GTK_LABEL(honey_label), buffer);

    // Water Label
    snprintf(buffer, sizeof(buffer), "Vesi täyttöön: <b>%.2f %s</b>", waterAmount, waterUnit);
    gtk_label_set_markup(GTK_LABEL(water_label), buffer);

    // Final Message Label
    if (is_turbo_mode == 2) {
        gtk_label_set_markup(GTK_LABEL(message_label), "<span foreground='red'>Laskelma valmis. (Turbo-hiiva: FG pakotettu 1.000)</span>");
    } else if (target_og <= 1.225) {
        gtk_label_set_text(GTK_LABEL(message_label), "Laskelma valmis.");
    }
}

/**
 * @brief Callback function when the 'Laske' button is clicked.
 */
static void on_calculate_button_clicked(GtkWidget *widget, gpointer data) {
    const char *volume_str = gtk_entry_get_text(GTK_ENTRY(volume_entry));
    const char *abv_str = gtk_entry_get_text(GTK_ENTRY(abv_entry));
    const char *unit_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(unit_combobox));
    const char *sweetness_str = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(sweetness_combobox));
    gboolean is_turbo_active = gtk_switch_get_active(GTK_SWITCH(turbo_switch));

    // Input validation
    double volume_val = atof(volume_str);
    int abv_val = atoi(abv_str);

    if (volume_val <= 0.0 || abv_val <= 0) {
        gtk_label_set_text(GTK_LABEL(message_label), "Virhe: Syötä kelvolliset tilavuus ja ABV.");
        return;
    }

    // Turbo mode: 1 (Standard) or 2 (Turbo)
    int is_turbo_mode = is_turbo_active ? 2 : 1;

    // If turbo is active, sweetness is ignored for calculation but we use the current selection for the prompt.
    // For calculation purposes, we only need the string value of sweetness for Standard mode (is_turbo_mode == 1).
    const char* calculated_sweetness_str = sweetness_str;
    if (is_turbo_mode == 2) {
        // In Turbo mode, the FG is always 1.000 (Dry)
        calculated_sweetness_str = "Dry";
    }

    calculate_ingredients(volume_val, abv_val, unit_str, calculated_sweetness_str, is_turbo_mode);
}

/**
 * @brief Creates the main application window and UI elements.
 */
static void activate(GtkApplication *app, gpointer user_data) {
    GtkWidget *grid;
    GtkWidget *button;
    GtkWidget *label;
    GtkWidget *hbox;

    // 1. Create Window and assign to global variable
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Mead Master Laskuri");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 450, 400);
    gtk_container_set_border_width(GTK_CONTAINER(main_window), 15);

    // 2. Create Main Grid (container for all elements)
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(main_window), grid);

    int row = 0;

    // --- Title ---
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='large' weight='bold' foreground='#8B4513'>Siman Ainesosalaskuri</span>");
    gtk_label_set_xalign(GTK_LABEL(label), 0.5);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    // --- Volume Input + Water Info Button ---
    label = gtk_label_new("Erän tilavuus:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    volume_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(volume_entry), "5.0");
    gtk_box_pack_start(GTK_BOX(hbox), volume_entry, TRUE, TRUE, 0);

    unit_combobox = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(unit_combobox), "Gallons");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(unit_combobox), "Liters");
    gtk_combo_box_set_active(GTK_COMBO_BOX(unit_combobox), 0);
    gtk_box_pack_start(GTK_BOX(hbox), unit_combobox, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Vesi-Info");
    g_signal_connect(button, "clicked", G_CALLBACK(on_water_info_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(grid), hbox, 1, row++, 1, 1);


    // --- ABV Input ---
    label = gtk_label_new("Tavoite ABV (%):");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    abv_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(abv_entry), "14");
    gtk_grid_attach(GTK_GRID(grid), abv_entry, 1, row++, 1, 1);

    // --- Sweetness Combobox + Honey Info Button ---
    label = gtk_label_new("Makeustaso:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    sweetness_combobox = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sweetness_combobox), "Dry");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sweetness_combobox), "Semi-Sweet");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sweetness_combobox), "Sweet");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(sweetness_combobox), "Dessert");
    gtk_combo_box_set_active(GTK_COMBO_BOX(sweetness_combobox), 1);
    gtk_box_pack_start(GTK_BOX(hbox), sweetness_combobox, TRUE, TRUE, 0);

    button = gtk_button_new_with_label("Hunaja-Info");
    g_signal_connect(button, "clicked", G_CALLBACK(on_honey_info_clicked), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

    gtk_grid_attach(GTK_GRID(grid), hbox, 1, row++, 1, 1);

    // --- Turbo Switch ---
    label = gtk_label_new("Käytä Turbo-hiivaa:");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row, 1, 1);
    turbo_switch = gtk_switch_new();
    gtk_switch_set_active(GTK_SWITCH(turbo_switch), FALSE);
    gtk_grid_attach(GTK_GRID(grid), turbo_switch, 1, row++, 1, 1);

    // --- Calculate Button ---
    button = gtk_button_new_with_label("Laske Ainesosat");
    g_signal_connect(button, "clicked", G_CALLBACK(on_calculate_button_clicked), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, row++, 2, 1);

    // --- Separator ---
    label = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    // --- Results Section ---
    label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(label), "<span size='medium' weight='bold' foreground='#A0522D'>LASKENTATULOKSET:</span>");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), label, 0, row++, 2, 1);

    // Results Labels initialization
    og_label = gtk_label_new("OG (Ominaispaino):");
    gtk_label_set_xalign(GTK_LABEL(og_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), og_label, 0, row++, 2, 1);

    fg_label = gtk_label_new("FG (Loppupaino):");
    gtk_label_set_xalign(GTK_LABEL(fg_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), fg_label, 0, row++, 2, 1);

    honey_label = gtk_label_new("Tarvittava hunaja:");
    gtk_label_set_xalign(GTK_LABEL(honey_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), honey_label, 0, row++, 2, 1);

    water_label = gtk_label_new("Vesi täyttöön:");
    gtk_label_set_xalign(GTK_LABEL(water_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), water_label, 0, row++, 2, 1);

    // Message Label (for warnings and errors)
    message_label = gtk_label_new("Paina 'Laske Ainesosat' nähdäksesi tulokset.");
    gtk_label_set_xalign(GTK_LABEL(message_label), 0.0);
    gtk_grid_attach(GTK_GRID(grid), message_label, 0, row++, 2, 1);

    // Initial calculation on startup to populate labels
    calculate_ingredients(5.0, 14, "Gallons", "Semi-Sweet", 1);


    // 3. Show Window
    gtk_widget_show_all(main_window);
}

// --- Main function for GTK application ---
int main(int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new("com.example.meadcalculator", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);

    return status;
}
