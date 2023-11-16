#include "form_rescalc.h"

#include <algorithm>

#include "ui_form_rescalc.h"

FormResCalc::FormResCalc(QWidget* parent) : QWidget(parent), ui(new Ui::FormResCalc)
{
    ui->setupUi(this);

    std::array<int, 24> base_values = {10, 11, 12, 13, 15, 16, 18, 20, 22, 24, 27, 30, 33, 36, 39, 43, 47, 51, 56, 62, 68, 75, 82, 91};

    std::array<int, 7> value_scales = {-1, 0, 1, 2, 3, 4, 5};

    for (const auto& sc : value_scales) {
        for (const auto& bv : base_values) {
            Resistor r;
            r.base_value = bv;
            r.scale = sc;
            all_resistors.push_back(r);
            // std::cout << r.to_string() << std::endl;
        }
    }

    connect(ui->pushButton_find_res, SIGNAL(clicked(bool)), SLOT(slot_find_resistor()));
    connect(ui->pushButton_find_div, SIGNAL(clicked(bool)), SLOT(slot_find_divider()));
}

FormResCalc::~FormResCalc() { delete ui; }

std::pair<bool, double> resistor_from_input(const QLineEdit* line_edit)
{
    double expect;
    QString resistance_str = line_edit->text();
    if (resistance_str.isEmpty()) return {false, 0.0};

    if (resistance_str.endsWith('M')) {
        resistance_str.chop(1);
        expect = resistance_str.toDouble();
        expect *= 1000000.0;
    } else if (resistance_str.endsWith('K') || resistance_str.endsWith('k')) {
        resistance_str.chop(1);
        expect = resistance_str.toDouble();
        expect *= 1000.0;
    } else if (resistance_str.endsWith('m')) {
        resistance_str.chop(1);
        expect = resistance_str.toDouble();
        expect /= 1000.0;
    } else {
        expect = resistance_str.toDouble();
    }
    return {true, expect};
}

void FormResCalc::slot_find_resistor()
{
    double err = ui->lineEdit_max_err->text().toDouble() / 100.0;

    if (sender() == ui->pushButton_find_res) {
        ui->lineEdit_r2->clear();
        ui->lineEdit_vout->clear();
        ui->lineEdit_vref->clear();
    }

    auto r = resistor_from_input(ui->lineEdit_expect_r);
    if (!r.first) return;

    double expect = r.second;

    std::vector<Combination> ret;
    ui->textBrowser->clear();
    if (!find_combination(expect, ret, err)) {
        ui->textBrowser->append(tr("No combination is found!!"));
    } else {
        std::sort(ret.rbegin(), ret.rend(), [](const auto& a, const auto& b) { return a.error > b.error; });
        int num_results = ui->spinBox_num_results->value();
        if (ret.size() > num_results) {
            ret.resize(num_results);
        }

        for (const auto& c : ret) {
            ui->textBrowser->append(tr(" ----------- "));
            ui->textBrowser->append(tr("Found combination:"));
            ui->textBrowser->append(tr("Type: %1 -- Value: %2")
                                        .arg(c.type == Combination::TYPE_SINGLE
                                                 ? tr("Single")
                                                 : (c.type == Combination::TYPE_SERIES ? tr("Series") : tr("Parallel")))
                                        .arg(c.actual));
            int i = 0;
            for (const auto& r : c.resistors) {
                ui->textBrowser->append(tr("Resistor%1: %2").arg(++i).arg(QString::fromStdString(r.to_string())));
            }
            ui->textBrowser->append(tr("Error: %1 \%").arg(c.error * 100.0));
        }
    }
}

void FormResCalc::slot_find_divider()
{
    double vref = ui->lineEdit_vref->text().toDouble();
    double vout = ui->lineEdit_vout->text().toDouble();
    double err = ui->lineEdit_max_err->text().toDouble() / 100.0;

    auto r2 = resistor_from_input(ui->lineEdit_r2);
    if (r2.first) {
        double r1 = (vout / vref) * r2.second - r2.second;
        ui->lineEdit_expect_r->setText(QString::number(r1));
        slot_find_resistor();
        return;
    }

    ui->lineEdit_expect_r->clear();

    std::vector<Combination> ret;
    ui->textBrowser->clear();
    if (!find_dividers(vout, vref, ret, err)) {
        ui->textBrowser->append(tr("No combination is found!!"));
    } else {
        std::sort(ret.rbegin(), ret.rend(), [](const auto& a, const auto& b) { return a.error > b.error; });
        int num_results = ui->spinBox_num_results->value();
        if (ret.size() > num_results) {
            ret.resize(num_results);
        }

        for (const auto& c : ret) {
            ui->textBrowser->append(tr(" ----------- "));
            ui->textBrowser->append(tr("Found combination for R2=%1:").arg(QString::fromStdString(c.r2.to_string())));
            ui->textBrowser->append(tr("R1 Type: %1 -- Value: %2 -- Actual vout: %3")
                                        .arg(c.type == Combination::TYPE_SINGLE
                                                 ? tr("Single")
                                                 : (c.type == Combination::TYPE_SERIES ? tr("Series") : tr("Parallel")))
                                        .arg(c.r1)
                                        .arg(c.actual));
            int i = 0;
            for (const auto& r : c.resistors) {
                ui->textBrowser->append(tr("Resistor%1: %2").arg(++i).arg(QString::fromStdString(r.to_string())));
            }
            ui->textBrowser->append(tr("Error: %1 \%").arg(c.error * 100.0));
        }
    }
}

bool FormResCalc::find_dividers(double vout, double vref, std::vector<Combination>& combinations, double err)
{
    // (vout*r2)/(r1+r2) = vref
    // (vout*r2) = vref*r1+vref*r2
    // vout*r2 - vref*r2 = vref * r1
    // r1 = (vout/vref)*r2 - r2
    bool ret = false;

    for (const auto& r : all_resistors) {
        // pick a R2 that is >= 8K and <= 30K
        double r2 = r.value();
        if (r2 < 8000.0) continue;
        if (r2 > 30000.0) break;

        double r1 = (vout / vref) * r2 - r2;
        std::vector<Combination> combi;

        if (find_combination(r1, combi, err)) {
            for (auto& c : combi) {
                double actual_vout = (vref / r2) * (c.actual + r2);
                double actual_err = fabs(actual_vout - vout) / vout;

                c.r1 = c.actual;
                c.r2 = r;
                c.actual = actual_vout;
                c.error = actual_err;
                combinations.push_back(c);
            }
            ret = true;
        }
    }

    return ret;
}

enum
{
    COMBI_ANY = 0,
    COMBI_SINGLE,
    COMBI_SERIES,
    COMBI_PARALLEL
};

bool FormResCalc::find_combination(double expected_value, std::vector<Combination>& combinations, double err)
{
    bool ret = false;
    auto best_err = err;

    // 1 resistor
    for (const Resistor& r : all_resistors) {
        double actual = r.value();
        double actual_err = fabs(expected_value - actual) / expected_value;

        if (actual_err < best_err) {
            best_err = actual_err;

            Combination c;
            c.resistors.clear();
            c.resistors.push_back(r);
            c.actual = actual;
            c.error = actual_err;
            c.type = Combination::TYPE_SINGLE;
            combinations.push_back(c);

            ret = true;
        }
    }

    int combi_mode = ui->cb_combi->currentIndex();
    if (combi_mode == COMBI_SINGLE) {
        return ret;
    }

    bool allow_series = combi_mode != COMBI_PARALLEL;
    bool allow_parallel = combi_mode != COMBI_SERIES;

    // 2 resistors
    for (const Resistor& r1 : all_resistors) {
        for (const Resistor& r2 : all_resistors) {
            if (allow_parallel)
            {
                double actual = r1.parallel_value_with(r2);
                double actual_err = fabs(expected_value - actual) / expected_value;
                if (actual_err < best_err) {
                    best_err = actual_err;

                    Combination c;
                    c.resistors.clear();
                    c.resistors.push_back(r1);
                    c.resistors.push_back(r2);
                    c.actual = actual;
                    c.error = actual_err;
                    c.type = Combination::TYPE_PARALLEL;
                    combinations.push_back(c);

                    ret = true;
                }
            }

            if (allow_series)
            {
                double actual = r1.value() + r2.value();
                double actual_err = fabs(expected_value - actual) / expected_value;
                if (actual_err < best_err) {
                    best_err = actual_err;

                    Combination c;
                    c.resistors.clear();
                    c.resistors.push_back(r1);
                    c.resistors.push_back(r2);
                    c.actual = actual;
                    c.error = actual_err;
                    c.type = Combination::TYPE_SERIES;
                    combinations.push_back(c);
                    ret = true;
                }
            }
        }
    }

    return ret;
}
