#ifndef FORM_RESCALC_H
#define FORM_RESCALC_H

#include <QWidget>

namespace Ui
{
class FormResCalc;
}

struct Resistor {
    int base_value = -1;
    int scale = 0;  // -1, 0, 1, 2, 3, 4, 5

    double value() const
    {
        if (scale == -1) {
            return base_value * 0.1;
        }

        int v = base_value;
        int s = scale;
        while (s-- > 0) {
            v *= 10;
        }
        return v;
    }

    double parallel_value_with(const Resistor& other) const
    {
        double v1 = 1.0 / this->value();
        double v2 = 1.0 / other.value();
        return 1.0 / (v1 + v2);
    }

    std::string to_string() const
    {
        char val[32] = {0};
        switch (scale) {
        case -1:
        case 2:
        case 5:
            snprintf(val, sizeof(val), "%.1f %sOhm", base_value * 0.1, scale == 5 ? "M " : (scale == 2 ? "K " : ""));
            break;
        case 0:
        case 1:
            snprintf(val, sizeof(val), "%d Ohm", base_value * (scale ? 10 : 1));
            break;
        default: {
            int v = base_value;
            int s = scale;
            while (s-- > 3) {
                v *= 10;
            }
            snprintf(val, sizeof(val), "%d K Ohm", v);
        } break;
        }
        return std::string(val);
    }
};

struct Combination {
    std::vector<Resistor> resistors;
    double actual;
    double error;

    double r1;
    Resistor r2;

    enum {
        TYPE_SINGLE,
        TYPE_SERIES,
        TYPE_PARALLEL,
    } type;
};

class FormResCalc : public QWidget
{
    Q_OBJECT

   public:
    explicit FormResCalc(QWidget* parent = nullptr);
    ~FormResCalc();

   private slots:
    void slot_find_resistor();
    void slot_find_divider();

   private:
    Ui::FormResCalc* ui;
    std::vector<Resistor> all_resistors;

    bool find_dividers(double vout, double vref, std::vector<Combination>& combinations, double err = 0.005);

    bool find_combination(double expected_value, std::vector<Combination>& combinations, double err = 0.005);
};

#endif  // FORM_RESCALC_H
