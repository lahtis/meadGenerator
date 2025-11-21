// Copyright (C) [2025] [Tuomas Lähteenmäki].
// The meadGenerator.c was created together with Gemini.
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.




#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// --- Constants ---

// The approximate gravity points per pound of honey per gallon of water (for calculation purposes).
// This is a standard estimate for most floral honeys (35 PPG).
#define GRAVITY_POINTS_PER_UNIT 35.0

// Conversion factors
#define KG_TO_LBS 2.20462 // 1 kg = 2.20462 lbs
#define L_TO_GAL 0.264172 // 1 L = 0.264172 gallons
#define VERSION_STRING  "0.1.1"

// --- Function Prototyypes ---
void display_menu();
double get_target_og(int abv, const char* sweetness, int is_turbo);
void calculate_us_imperial(double volume_gal, double target_og);
void calculate_metric(double volume_L, double target_og);
double convert_kg_to_lbs(double kg); // Note: Only used for conversion factor definition now
double convert_L_to_gal(double L);

// --- Main Application ---
int main() {
    display_menu();

    int choice;
    printf("\nSelect unit system (1 for US Imperial, 2 for Metric): ");
    if (scanf("%d", &choice) != 1 || (choice != 1 && choice != 2)) {
        printf("Invalid selection. Exiting.\n");
        return 1;
    }

    double volume;
    int abv;
    char sweetness_str[20];
    int is_turbo_mode; // New variable to track turbo mode

    // Get batch volume
    printf("Enter batch volume (in %s): ", (choice == 1) ? "Gallons" : "Liters");
    if (scanf("%lf", &volume) != 1 || volume <= 0.0) {
        printf("Invalid volume. Exiting.\n");
        return 1;
    }

    // Get target ABV
    printf("Enter target ABV (%%, e.g., 14): ");
    // Max ABV is now 25 to support turbo yeast
    if (scanf("%d", &abv) != 1 || abv < 5 || abv > 25) {
        printf("Invalid ABV range (must be between 5%% and 25%%). Exiting.\n");
        return 1;
    }

    // Get sweetness level
    printf("Enter sweetness level (Dry, Semi-Sweet, Sweet, Dessert): ");
    if (scanf("%s", sweetness_str) != 1) {
        printf("Invalid sweetness input. Exiting.\n");
        return 1;
    }

    // New: Ask for Turbo Yeast method
    printf("Are you using Turbo Yeast Method? (1 for Standard Yeast, 2 for Turbo Yeast): ");
    if (scanf("%d", &is_turbo_mode) != 1 || (is_turbo_mode != 1 && is_turbo_mode != 2)) {
        printf("Invalid selection for yeast method. Exiting.\n");
        return 1;
    }


    // Calculate Target Original Gravity (OG)
    double target_og = get_target_og(abv, sweetness_str, is_turbo_mode);

    if (target_og == 0.0) {
        printf("Error: Invalid sweetness level entered. Please use Dry, Semi-Sweet, Sweet, or Dessert.\n");
        return 1;
    }

    // Sanity Check: If calculated OG is unrealistically high, stop and warn the user.
    if (target_og > 1.225) {
        printf("\nWARNING: Calculated Original Gravity (OG=%.3f) is extremely high.\n", target_og);
        printf("This OG requires an impractical amount of honey and exceeds the tolerance of most mead yeasts (max OG is usually around 1.220).\n");
        printf("Please try a lower ABV or a smaller batch size.\n");
        return 1;
    }

    // Perform calculation based on chosen unit system
    printf("\n--- Calculation Results ---\n");
    if (choice == 1) {
        calculate_us_imperial(volume, target_og);
    } else {
        calculate_metric(volume, target_og);
    }

    printf("\nCalculation complete. Remember this is an ESTIMATE and specific yeast/flavorings are required.\n");
    return 0;
}

// --- Function Definitions ---

/**
 * @brief Displays the introductory menu and context.
 */
void display_menu() {
    printf("======================================\n");
    printf("      C Mead Ingredients Calculator\n");
    printf("======================================\n");
    printf("This tool calculates the approximate amount of honey needed to reach a\n");
    printf("target Original Gravity (OG) based on your desired ABV and sweetness.\n");
    printf("Assumptions:\n");
    printf(" - Honey contributes 35 gravity points per pound per gallon (PPG).\n");
    printf(" - Sweetness level determines the assumed Final Gravity (FG).\n");
    printf(" - TURBO YEAST MODE: Forces Final Gravity (FG) to 1.000 (Dry).\n");
}

/**
 * @brief Calculates the required Original Gravity (OG) based on target ABV and sweetness.
 * * @param abv Target Alcohol by Volume percentage.
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
        printf("\nNOTE: Turbo Yeast selected. Final Gravity (FG) forced to 1.000.\n");
    }


    // Mead/Wine ABV formula approximation: ABV = (OG - FG) * 131.25
    // Rearranging for OG: OG = FG + (ABV / 131.25)
    double og = fg + ((double)abv / 131.25);

    // OG is represented as 1.XXX, so we round the result
    return round(og * 1000.0) / 1000.0;
}

/**
 * @brief Performs calculations in US Imperial units (Gallons/Lbs).
 * * @param volume_gal Batch volume in Gallons.
 * @param target_og The calculated Original Gravity.
 */
void calculate_us_imperial(double volume_gal, double target_og) {
    // Gravity Points needed = (Target OG - 1.000) * 1000
    double gravity_points_needed = (target_og - 1.000) * 1000.0 * volume_gal;

    // Honey Lbs = Gravity Points needed / Gravity Points per unit
    double honey_lbs = gravity_points_needed / GRAVITY_POINTS_PER_UNIT;

    // Water volume: assume honey displaces 0.65 gallons per 10 lbs.
    double honey_volume_gal = honey_lbs / 10.0 * 0.65;
    double water_gal = volume_gal - honey_volume_gal;

    printf("Target Original Gravity (OG): %.3f\n", target_og);
    printf("Required Honey:             %.2f lbs (pounds)\n", honey_lbs);

    // Added clarification for when water volume is zero or negative
    if (water_gal <= 0.0) {
        printf("Required Water (to top off):  0.00 gallons (Honey volume meets or exceeds batch volume.)\n");
    } else {
        printf("Required Water (to top off):  %.2f gallons\n", water_gal);
    }

    printf("Total Gravity Points Needed:  %.0f\n", gravity_points_needed);
}

/**
 * @brief Performs calculations and converts output to Metric units (Liters/Kg).
 * * @param volume_L Batch volume in Liters.
 * @param target_og The calculated Original Gravity.
 */
void calculate_metric(double volume_L, double target_og) {
    // Convert target volume to gallons for consistent calculation using PPG constant
    double volume_gal = volume_L * L_TO_GAL;

    // Calculate honey needed in pounds (Lbs) using the US Imperial function's logic
    double gravity_points_needed = (target_og - 1.000) * 1000.0 * volume_gal;
    double honey_lbs_calc = gravity_points_needed / GRAVITY_POINTS_PER_UNIT;

    // Convert honey from Lbs to Kilograms (CORRECTION APPLIED HERE: Division instead of multiplication)
    double honey_kg = honey_lbs_calc / KG_TO_LBS;

    // Honey volume: assume 1 kg displaces ~0.74 Liters
    double honey_volume_L = honey_kg * 0.74;
    double water_L = volume_L - honey_volume_L;

    printf("Target Original Gravity (OG): %.3f\n", target_og);
    printf("Required Honey:             %.2f kg (kilograms)\n", honey_kg);

    // Added clarification for when water volume is zero or negative
    if (water_L <= 0.0) {
        printf("Required Water (to top off):  0.00 liters (Honey volume meets or exceeds batch volume.)\n");
    } else {
        printf("Required Water (to top off):  %.2f liters\n", water_L);
    }

    printf("Total Gravity Points Needed:  %.0f (Based on US Gal/Lbs)\n", gravity_points_needed);
}

/**
 * @brief Converts Kilograms (kg) to Pounds (lbs).
 * NOTE: This function is not used in metric calculation after the fix, but kept for clarity.
 */
double convert_kg_to_lbs(double kg) {
    return kg * KG_TO_LBS;
}

/**
 * @brief Converts Liters (L) to Gallons (gal).
 */
double convert_L_to_gal(double L) {
    return L * L_TO_GAL;
}
