#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

/* --- PLATFORM-DEPENDENT SLEEP FUNCTION --- */
#ifdef _WIN32
    #include <windows.h>
    #define SLEEP_FUNCTION(ms) Sleep(ms)
    #define CLEAR_COMMAND "cls"
#else
    #include <unistd.h>
    #define SLEEP_FUNCTION(ms) usleep((ms) * 1000)
    #define CLEAR_COMMAND "clear"
#endif

/* --- SYSTEM CONSTANTS & DEFINITIONS --- */
#define MAX_EMPLOYEES 50
#define ADMIN_PIN 1234
#define FILENAME "mcdoPayrollRecords_MonthlyAttendance.txt"
#define ATTENDANCE_FILE "employee_attendance.txt"
#define MAX_STR 100
#define STANDARD_WORKING_DAYS 22
#define MAX_WORKING_DAYS 30
#define MAX_ATTENDANCE_RECORDS 1000

/* --- DEDUCTION CONSTANTS --- */
#define DEDUCTION_SSS_RATE 0.045f
#define DEDUCTION_PHILHEALTH_RATE 0.025f
#define DEDUCTION_PAGIBIG_RATE 0.02f
#define OVERTIME_RATE_MULTIPLIER 1.25f

/* --- INCOME TAX TIERS (Updated 2024 Philippine Tax Table) --- */
#define TAX_TIER_1_LIMIT 20833.00f
#define TAX_TIER_2_LIMIT 33333.00f
#define TAX_TIER_3_LIMIT 66667.00f
#define TAX_TIER_4_LIMIT 166667.00f
#define TAX_TIER_5_LIMIT 666667.00f

#define TAX_RATE_TIER_2 0.15f
#define TAX_RATE_TIER_3 0.20f
#define TAX_RATE_TIER_4 0.25f
#define TAX_RATE_TIER_5 0.30f
#define TAX_RATE_TIER_6 0.35f

#define TAX_BASE_TIER_2 0.00f
#define TAX_BASE_TIER_3 2500.00f
#define TAX_BASE_TIER_4 10833.33f
#define TAX_BASE_TIER_5 40833.33f
#define TAX_BASE_TIER_6 200833.33f

/* --- POSITION DEFINITIONS --- */
typedef enum { SERVICE_CREW, COOKER, COUNTER_CREW, NUM_POSITIONS } PositionType;

const char *PositionNames[NUM_POSITIONS] = { "Service Crew", "Cooker", "Counter Crew" };
const float PositionMonthlySalaries[NUM_POSITIONS] = {
    18000.00f,
    25000.00f,
    35000.00f
};

/* --- ATTENDANCE STRUCTURES --- */
typedef struct {
    int empID;
    char date[11];
    char timeIn[6];
    float hoursWorked;
    char status[20];
    int isLate;
    float overtimeHours;
} AttendanceRecord;

/* --- EMPLOYEE STRUCTURE --- */
typedef struct {
    int empID;
    char name[50];
    PositionType position;
    float monthlySalary;
    int daysWorked;
    float totalOvertimeHours;
    float totalHoursWorked;
    float lastOvertimePay;
    float lastDailyRate;
    float lastAbsentDeduct;
    float lastGrossPay;
    float lastNetPay;
    float lastSSS;
    float lastPhilHealth;
    float lastPagIBIG;
    float lastIncomeTax;
} Employee;

/* --- GLOBAL STORAGE --- */
Employee employees[MAX_EMPLOYEES];
int employeeCount = 0;

AttendanceRecord attendanceRecords[MAX_ATTENDANCE_RECORDS];
int attendanceCount = 0;

/* --- FUNCTION PROTOTYPES --- */
void clearInputBuffer(void);
void pressEnterToContinue(void);
int getIntInput(const char *prompt, int min, int max);
void getStringInput(const char *prompt, char *out, int maxlen, int letters_spaces_only);
int findEmployeeIndexByID(int id);
int findEmployeeIndexByName(const char *name);
PositionType getPositionTypeByName(const char *name);
void saveToFile(void);
void loadFromFile(void);
void saveAttendanceToFile(void);
void loadAttendanceFromFile(void);
void printPaySlipToFile(const Employee *e);
void mainMenu(void);
void adminMenu(void);
void viewEmployees(void);
void addEmployee(void);
void updateEmployee(void);
void removeEmployee(void);
void calculateAndDisplaySalary(void);
float calculateIncomeTax(float taxableIncome);
void displayEmployeeSalarySlip(int id);
void recordTimeIn(void);
void calculateAttendanceSummary(void);
void generateMonthlyAttendanceReport(void);
int generateEmployeeID(void);
int isNameDuplicate(const char *name, int excludeID);

/* --- IMPLEMENTATION --- */

void clearInputBuffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

void pressEnterToContinue(void) {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    clearInputBuffer();
}

int generateEmployeeID(void) {
    // Generate random 7-digit ID between 1000000 and 9999999
    int id;
    int attempts = 0;
    int max_attempts = 100;
    
    srand(time(NULL));
    
    do {
        id = 1000000 + (rand() % 9000000);
        attempts++;
        
        if (attempts >= max_attempts) {
            // Fallback: find the next available ID
            int max_id = 1000000;
            for (int i = 0; i < employeeCount; i++) {
                if (employees[i].empID > max_id) {
                    max_id = employees[i].empID;
                }
            }
            id = max_id + 1;
            if (id > 9999999) {
                id = 1000000;
            }
            break;
        }
    } while (findEmployeeIndexByID(id) != -1);
    
    return id;
}

int findEmployeeIndexByID(int id) {
    for (int i = 0; i < employeeCount; ++i) {
        if (employees[i].empID == id) return i;
    }
    return -1;
}

int findEmployeeIndexByName(const char *name) {
    for (int i = 0; i < employeeCount; ++i) {
        if (strcasecmp(employees[i].name, name) == 0) return i;
    }
    return -1;
}

// Case-insensitive string comparison
int strcasecmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        int diff = tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
        if (diff != 0) return diff;
        s1++;
        s2++;
    }
    return tolower((unsigned char)*s1) - tolower((unsigned char)*s2);
}

int isNameDuplicate(const char *name, int excludeID) {
    for (int i = 0; i < employeeCount; ++i) {
        if (employees[i].empID != excludeID && strcasecmp(employees[i].name, name) == 0) {
            return 1;
        }
    }
    return 0;
}

PositionType getPositionTypeByName(const char *name) {
    for (int i = 0; i < NUM_POSITIONS; i++) {
        if (strcmp(name, PositionNames[i]) == 0) {
            return (PositionType)i;
        }
    }
    return SERVICE_CREW;
}

int getIntInput(const char *prompt, int min, int max) {
    int value;
    while (1) {
        if (prompt) printf("%s", prompt);
        if (scanf("%d", &value) != 1) {
            printf("Invalid input. Please enter a valid number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();
        if (value < min || value > max) {
            printf("Please enter a value between %d and %d.\n", min, max);
            continue;
        }
        return value;
    }
}

void getStringInput(const char *prompt, char *out, int maxlen, int letters_spaces_only) {
    while (1) {
        if (prompt) printf("%s", prompt);
        if (!fgets(out, maxlen, stdin)) {
            clearInputBuffer();
            printf("Input error. Please try again.\n");
            continue;
        }
        size_t n = strlen(out);
        if (n > 0 && out[n-1] == '\n') out[n-1] = '\0';
        else if (n == (size_t)maxlen - 1) clearInputBuffer();
        
        char *start = out;
        while (*start && isspace((unsigned char)*start)) start++;
        char *end = out + strlen(out) - 1;
        while (end > start && isspace((unsigned char)*end)) end--;
        *(end + 1) = '\0';
        
        if (start != out) {
            memmove(out, start, strlen(start) + 1);
        }
        
        if (strlen(out) == 0) {
            printf("Input cannot be empty or just spaces.\n");
            continue;
        }
        
        if (letters_spaces_only) {
            int ok = 1;
            int has_letter = 0;
            for (size_t i = 0; i < strlen(out); ++i) {
                if (isalpha((unsigned char)out[i])) {
                    has_letter = 1;
                } else if (out[i] != ' ' && out[i] != '-' && out[i] != '.') {
                    ok = 0;
                    break;
                }
            }
            if (!ok) {
                printf("Invalid characters detected. Use only letters, spaces, hyphens, or periods.\n");
                continue;
            }
            if (!has_letter) {
                printf("Name must contain at least one letter.\n");
                continue;
            }
        }
        
        if (strlen(out) < 2) {
            printf("Name must be at least 2 characters long.\n");
            continue;
        }
        
        if (strlen(out) > 48) {
            printf("Name is too long. Maximum 48 characters.\n");
            continue;
        }
        
        return;
    }
}

void getCurrentDateTime(char *date, char *timeBuf) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(date, "%04d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
    sprintf(timeBuf, "%02d:%02d", tm.tm_hour, tm.tm_min);
}

/* --- FILE HANDLING --- */

void saveToFile(void) {
    FILE *fp = fopen(FILENAME, "w");
    if (!fp) {
        printf("\nError: Unable to save data to %s\n", FILENAME);
        return;
    }
    fprintf(fp, "%d\n", employeeCount);
    for (int i = 0; i < employeeCount; i++) {
        fprintf(fp, "%d\n%s\n%s\n%.2f\n%d\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n%.2f\n",
            employees[i].empID,
            employees[i].name,
            PositionNames[employees[i].position],
            employees[i].monthlySalary,
            employees[i].daysWorked,
            employees[i].totalOvertimeHours,
            employees[i].totalHoursWorked,
            employees[i].lastOvertimePay,
            employees[i].lastDailyRate,
            employees[i].lastAbsentDeduct,
            employees[i].lastGrossPay,
            employees[i].lastNetPay,
            employees[i].lastSSS,
            employees[i].lastPhilHealth,
            employees[i].lastPagIBIG,
            employees[i].lastIncomeTax
        );
    }
    fclose(fp);
}

void loadFromFile(void) {
    FILE *fp = fopen(FILENAME, "r");
    if (!fp) {
        printf("No existing payroll file found. Starting fresh.\n");
        employeeCount = 0;
        return;
    }
    if (fscanf(fp, "%d\n", &employeeCount) != 1) {
        printf("Error reading employee count from file. Starting fresh.\n");
        employeeCount = 0;
        fclose(fp);
        return;
    }

    char posName[50];
    for (int i = 0; i < employeeCount && i < MAX_EMPLOYEES; i++) {
        if (fscanf(fp, "%d\n", &employees[i].empID) != 1) break;

        if (!fgets(employees[i].name, sizeof(employees[i].name), fp)) break;
        employees[i].name[strcspn(employees[i].name, "\n")] = 0;

        if (!fgets(posName, sizeof(posName), fp)) break;
        posName[strcspn(posName, "\n")] = 0;
        employees[i].position = getPositionTypeByName(posName);

        if (fscanf(fp, "%f\n%d\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n%f\n",
            &employees[i].monthlySalary,
            &employees[i].daysWorked,
            &employees[i].totalOvertimeHours,
            &employees[i].totalHoursWorked,
            &employees[i].lastOvertimePay,
            &employees[i].lastDailyRate,
            &employees[i].lastAbsentDeduct,
            &employees[i].lastGrossPay,
            &employees[i].lastNetPay,
            &employees[i].lastSSS,
            &employees[i].lastPhilHealth,
            &employees[i].lastPagIBIG,
            &employees[i].lastIncomeTax) != 13) break;
    }

    if (employeeCount > 0) {
        printf("Loaded %d employees and their payroll data from file.\n", employeeCount);
    }
    fclose(fp);
}

void saveAttendanceToFile(void) {
    FILE *fp = fopen(ATTENDANCE_FILE, "w");
    if (!fp) {
        printf("\nError: Unable to save attendance data to %s\n", ATTENDANCE_FILE);
        return;
    }
    fprintf(fp, "%d\n", attendanceCount);
    for (int i = 0; i < attendanceCount; i++) {
        fprintf(fp, "%d\n%s\n%s\n%.2f\n%s\n%d\n%.2f\n",
            attendanceRecords[i].empID,
            attendanceRecords[i].date,
            attendanceRecords[i].timeIn,
            attendanceRecords[i].hoursWorked,
            attendanceRecords[i].status,
            attendanceRecords[i].isLate,
            attendanceRecords[i].overtimeHours
        );
    }
    fclose(fp);
}

void loadAttendanceFromFile(void) {
    FILE *fp = fopen(ATTENDANCE_FILE, "r");
    if (!fp) {
        printf("No existing attendance file found. Starting fresh.\n");
        attendanceCount = 0;
        return;
    }
    if (fscanf(fp, "%d\n", &attendanceCount) != 1) {
        printf("Error reading attendance count from file. Starting fresh.\n");
        attendanceCount = 0;
        fclose(fp);
        return;
    }

    for (int i = 0; i < attendanceCount && i < MAX_ATTENDANCE_RECORDS; i++) {
        if (fscanf(fp, "%d\n", &attendanceRecords[i].empID) != 1) break;

        if (!fgets(attendanceRecords[i].date, sizeof(attendanceRecords[i].date), fp)) break;
        attendanceRecords[i].date[strcspn(attendanceRecords[i].date, "\n")] = 0;

        if (!fgets(attendanceRecords[i].timeIn, sizeof(attendanceRecords[i].timeIn), fp)) break;
        attendanceRecords[i].timeIn[strcspn(attendanceRecords[i].timeIn, "\n")] = 0;

        if (fscanf(fp, "%f\n", &attendanceRecords[i].hoursWorked) != 1) break;

        if (!fgets(attendanceRecords[i].status, sizeof(attendanceRecords[i].status), fp)) break;
        attendanceRecords[i].status[strcspn(attendanceRecords[i].status, "\n")] = 0;

        if (fscanf(fp, "%d\n%f\n", 
            &attendanceRecords[i].isLate,
            &attendanceRecords[i].overtimeHours) != 2) break;
    }

    if (attendanceCount > 0) {
        printf("Loaded %d attendance records from file.\n", attendanceCount);
    }
    fclose(fp);
}

void printPaySlipToFile(const Employee *e) {
    char filename[MAX_STR];
    sprintf(filename, "payslip_%d.txt", e->empID);

    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("\nERROR: Unable to create file %s.\n", filename);
        return;
    }

    float totalDeduction = e->lastSSS + e->lastPhilHealth + e->lastPagIBIG + e->lastIncomeTax;
    float actualBasicPay = e->lastDailyRate * e->daysWorked;

    fprintf(fp, "================================================\n");
    fprintf(fp, "         OFFICIAL MONTHLY SALARY SLIP          \n");
    fprintf(fp, "================================================\n");
    fprintf(fp, "Employee ID:               %d\n", e->empID);
    fprintf(fp, "Employee Name:             %s\n", e->name);
    fprintf(fp, "Position:                  %s\n", PositionNames[e->position]);
    fprintf(fp, "Fixed Monthly Salary Base: Php%.2f\n", e->monthlySalary);
    fprintf(fp, "Standard Working Days:     %d\n", STANDARD_WORKING_DAYS);
    fprintf(fp, "Daily Rate:                Php%.2f\n", e->lastDailyRate);
    fprintf(fp, "Days Worked:               %d\n", e->daysWorked);
    fprintf(fp, "Days Absent:               %d\n", STANDARD_WORKING_DAYS - e->daysWorked);
    fprintf(fp, "------------------------------------------------\n");
    fprintf(fp, "EARNINGS:\n");
    fprintf(fp, "  - Basic Salary (Days Worked):    Php%.2f\n", actualBasicPay);
    if (e->lastOvertimePay > 0.0f) {
        fprintf(fp, "  - Overtime Pay (%.2f hrs @ %.2fx): +Php%.2f\n", 
                e->totalOvertimeHours, OVERTIME_RATE_MULTIPLIER, e->lastOvertimePay);
    }
    if (e->lastAbsentDeduct > 0) {
        fprintf(fp, "  - Less: Absent Deduction:        -Php%.2f\n", e->lastAbsentDeduct);
    }
    fprintf(fp, "GROSS PAY:                         Php%.2f\n", e->lastGrossPay);
    fprintf(fp, "------------------------------------------------\n");
    fprintf(fp, "MANDATORY DEDUCTIONS:\n");
    fprintf(fp, "  - SSS (4.5%%):                  -Php%.2f\n", e->lastSSS);
    fprintf(fp, "  - PhilHealth (2.5%%):           -Php%.2f\n", e->lastPhilHealth);
    fprintf(fp, "  - Pag-IBIG (2%%):               -Php%.2f\n", e->lastPagIBIG);
    fprintf(fp, "  - Withholding Tax:             -Php%.2f\n", e->lastIncomeTax);
    fprintf(fp, "------------------------------------------------\n");
    fprintf(fp, "TOTAL DEDUCTIONS:                -Php%.2f\n", totalDeduction);
    fprintf(fp, "NET SALARY (Take Home):            Php%.2f\n", e->lastNetPay);
    fprintf(fp, "================================================\n");

    fclose(fp);
    printf("\nSalary Slip successfully saved to file: %s\n", filename);
}

/* --- ATTENDANCE SYSTEM IMPLEMENTATION --- */

void recordTimeIn(void) {
    system(CLEAR_COMMAND);
    printf("\n=== EMPLOYEE TIME-IN ===\n");
    
    if (employeeCount == 0) {
        printf("No employees registered in the system.\n");
        return;
    }
    
    int empID;
    printf("Enter Employee ID: ");
    if (scanf("%d", &empID) != 1) {
        printf("Invalid ID format. Please enter numbers only.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();
    
    if (empID < 1000000 || empID > 9999999) {
        printf("Invalid Employee ID. Must be 7 digits (1000000-9999999).\n");
        return;
    }
    
    int empIndex = findEmployeeIndexByID(empID);
    
    if (empIndex == -1) {
        printf("Employee ID not found. Please check your ID and try again.\n");
        return;
    }
    
    char currentDate[11], currentTime[6];
    getCurrentDateTime(currentDate, currentTime);
    
    // Check if already timed in today
    for (int i = 0; i < attendanceCount; i++) {
        if (attendanceRecords[i].empID == empID && 
            strcmp(attendanceRecords[i].date, currentDate) == 0) {
            printf("You have already timed in today at %s.\n", attendanceRecords[i].timeIn);
            return;
        }
    }
    
    // Create new attendance record
    if (attendanceCount < MAX_ATTENDANCE_RECORDS) {
        AttendanceRecord newRecord = {0};
        newRecord.empID = empID;
        strcpy(newRecord.date, currentDate);
        strcpy(newRecord.timeIn, currentTime);
        newRecord.hoursWorked = 8.0f;
        strcpy(newRecord.status, "Present");
        
        // Check if late (after 8:00 AM)
        int hour, minute;
        sscanf(currentTime, "%d:%d", &hour, &minute);
        if (hour > 8 || (hour == 8 && minute > 0)) {
            newRecord.isLate = 1;
            strcpy(newRecord.status, "Late");
        }
        
        attendanceRecords[attendanceCount++] = newRecord;
        saveAttendanceToFile();
        
        printf("\nTime-In Recorded Successfully!\n");
        printf("Employee: %s\n", employees[empIndex].name);
        printf("Date: %s\n", currentDate);
        printf("Time-In: %s\n", currentTime);
        printf("Status: %s\n", newRecord.status);
        printf("Hours Worked: 8.0 (Standard)\n");
    } else {
        printf("Attendance records limit reached. Cannot record time-in.\n");
    }
}

void calculateAttendanceSummary(void) {
    system(CLEAR_COMMAND);
    printf("\n=== ATTENDANCE SUMMARY (Calculated for Payroll) ===\n");
    
    if (employeeCount == 0) {
        printf("No employees registered in the system.\n");
        return;
    }
    
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
    printf("| %-7s | %-15s | %-12s | %-8s | %-8s | %-15s | %-15s |\n", 
           "ID", "Name", "Days Worked", "Absent", "Late", "Overtime Hours", "Total Hours");
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
    
    for (int i = 0; i < employeeCount; i++) {
        int daysWorked = 0;
        int lateCount = 0;
        float totalOvertime = 0.0;
        float totalHours = 0.0;
        
        for (int j = 0; j < attendanceCount; j++) {
            if (attendanceRecords[j].empID == employees[i].empID) {
                daysWorked++;
                totalHours += attendanceRecords[j].hoursWorked;
                totalOvertime += attendanceRecords[j].overtimeHours;
                
                if (attendanceRecords[j].isLate) {
                    lateCount++;
                }
            }
        }
        
        employees[i].daysWorked = daysWorked;
        employees[i].totalOvertimeHours = totalOvertime;
        employees[i].totalHoursWorked = totalHours;
        
        printf("| %-7d | %-15.15s | %-12d | %-8d | %-8d | %-15.2f | %-15.2f |\n",
               employees[i].empID,
               employees[i].name,
               daysWorked,
               STANDARD_WORKING_DAYS - daysWorked,
               lateCount,
               totalOvertime,
               totalHours);
    }
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
    
    saveToFile();
    printf("\nAttendance summary calculated and saved.\n");
}

/* --- SALARY COMPUTATION LOGIC --- */

float calculateIncomeTax(float taxableIncome) {
    if (taxableIncome <= TAX_TIER_1_LIMIT) {
        return 0.00f;
    } else if (taxableIncome <= TAX_TIER_2_LIMIT) {
        return (taxableIncome - TAX_TIER_1_LIMIT) * TAX_RATE_TIER_2;
    } else if (taxableIncome <= TAX_TIER_3_LIMIT) {
        return TAX_BASE_TIER_3 + (taxableIncome - TAX_TIER_2_LIMIT) * TAX_RATE_TIER_3;
    } else if (taxableIncome <= TAX_TIER_4_LIMIT) {
        return TAX_BASE_TIER_4 + (taxableIncome - TAX_TIER_3_LIMIT) * TAX_RATE_TIER_4;
    } else if (taxableIncome <= TAX_TIER_5_LIMIT) {
        return TAX_BASE_TIER_5 + (taxableIncome - TAX_TIER_4_LIMIT) * TAX_RATE_TIER_5;
    } else {
        return TAX_BASE_TIER_6 + (taxableIncome - TAX_TIER_5_LIMIT) * TAX_RATE_TIER_6;
    }
}

void calculateAndDisplaySalary(void) {
    system(CLEAR_COMMAND);
    if (employeeCount == 0) {
        printf("\nNo employees for salary computation.\n");
        return;
    }

    printf("\n=== Monthly Salary Computation (Attendance-Based) ===\n");
    printf("(Based on %d Standard Working Days)\n\n", STANDARD_WORKING_DAYS);

    printf("+---------+-----------------+-------+------------+-------------+--------------+-------------+\n");
    printf("| %-7s | %-15s | %-5s | %-10s | %-11s | %-12s | %-11s |\n", 
           "ID", "Name", "Days", "Daily Rate", "Basic Salary", "Deductions", "Net Salary");
    printf("+---------+-----------------+-------+------------+-------------+--------------+-------------+\n");

    for (int i = 0; i < employeeCount; i++) {
        // Calculate rates
        float dailyRate = employees[i].monthlySalary / STANDARD_WORKING_DAYS;
        float hourlyRate = dailyRate / 8.0f;

        // Calculate Basic Pay and Absent Deduction
        float basicSalary = dailyRate * employees[i].daysWorked;
        
        float absentDeduct = 0.0f;
        if (employees[i].daysWorked < STANDARD_WORKING_DAYS) {
            absentDeduct = (STANDARD_WORKING_DAYS - employees[i].daysWorked) * dailyRate;
        }

        // Calculate Overtime Pay
        float overtimePay = employees[i].totalOvertimeHours * hourlyRate * OVERTIME_RATE_MULTIPLIER;

        // Calculate Gross Pay (Basic salary minus absent deduction plus overtime)
        float grossPay = (basicSalary - absentDeduct) + overtimePay;

        // Calculate mandatory deductions (based on gross pay)
        float sssDeduct = grossPay * DEDUCTION_SSS_RATE;
        float philhealthDeduct = grossPay * DEDUCTION_PHILHEALTH_RATE;
        float pagibigDeduct = grossPay * DEDUCTION_PAGIBIG_RATE;
        
        // Calculate taxable income (gross pay minus non-taxable deductions)
        // Note: SSS, PhilHealth, and Pag-IBIG are non-taxable
        float taxableIncome = grossPay;
        
        // Calculate Income Tax
        float incomeTaxDeduct = calculateIncomeTax(taxableIncome);

        // Final calculation
        float totalDeduction = sssDeduct + philhealthDeduct + pagibigDeduct + incomeTaxDeduct;
        float netSalary = grossPay - totalDeduction;

        // Store calculation results
        employees[i].lastDailyRate = dailyRate;
        employees[i].lastAbsentDeduct = absentDeduct;
        employees[i].lastOvertimePay = overtimePay;
        employees[i].lastGrossPay = grossPay;
        employees[i].lastNetPay = netSalary;
        employees[i].lastSSS = sssDeduct;
        employees[i].lastPhilHealth = philhealthDeduct;
        employees[i].lastPagIBIG = pagibigDeduct;
        employees[i].lastIncomeTax = incomeTaxDeduct;

        printf("| %-7d | %-15.15s | %-5d | %-10.2f | %-11.2f | %-12.2f | %-11.2f |\n",
               employees[i].empID, 
               employees[i].name, 
               employees[i].daysWorked,
               dailyRate,
               basicSalary - absentDeduct, 
               totalDeduction,
               netSalary);
    }
    printf("+---------+-----------------+-------+------------+-------------+--------------+-------------+\n");
    saveToFile();
    printf("\nAll monthly salary computations completed and saved.\n");
}

void displayEmployeeSalarySlip(int id) {
    system(CLEAR_COMMAND);
    int idx = findEmployeeIndexByID(id);

    if (idx == -1) {
        printf("\nEmployee with ID %d not found.\n", id);
        return;
    }
    
    if (employees[idx].lastGrossPay == 0.0f && employees[idx].daysWorked == 0) {
        printf("\nSalary computation has not been run for this employee yet.\n");
        printf("Please run 'Calculate & View Monthly Salary Computation' first.\n");
        return;
    }

    printf("\nGenerating Salary Slip...\n");
    SLEEP_FUNCTION(500);

    Employee e = employees[idx];

    printf("\n========= MONTHLY SALARY SLIP =========\n");
    printf("Employee ID: %d\n", e.empID);
    printf("Employee Name: %s\n", e.name);
    printf("Position: %s\n", PositionNames[e.position]);
    printf("Monthly Salary Base: Php%.2f\n", e.monthlySalary);
    printf("Daily Rate: Php%.2f\n", e.lastDailyRate);
    printf("Days Worked: %d / %d\n", e.daysWorked, STANDARD_WORKING_DAYS);
    printf("Overtime Hours: %.2f\n", e.totalOvertimeHours);
    printf("-----------------------------------------\n");

    float totalDeduction = e.lastSSS + e.lastPhilHealth + e.lastPagIBIG + e.lastIncomeTax;
    float actualBasicPay = e.lastDailyRate * e.daysWorked;

    printf("EARNINGS:\n"); 
    printf("- Basic Salary (Days Worked):   Php%.2f\n", actualBasicPay);
    if (e.lastOvertimePay > 0.0f) {
        printf("- ADD: Overtime Pay (%.2fx):    +Php%.2f\n", OVERTIME_RATE_MULTIPLIER, e.lastOvertimePay);
    }
    if (e.lastAbsentDeduct > 0) {
        printf("- Less: Absent Deduction:        -Php%.2f\n", e.lastAbsentDeduct); 
    }
    printf("-----------------------------------------\n");
    printf("TOTAL GROSS PAY:                Php%.2f\n", e.lastGrossPay); 
    printf("-----------------------------------------\n");
    printf("DEDUCTIONS:\n");
    printf("- SSS (4.5%%):           -Php%.2f\n", e.lastSSS);
    printf("- PhilHealth (2.5%%):     -Php%.2f\n", e.lastPhilHealth);
    printf("- Pag-IBIG (2%%):         -Php%.2f\n", e.lastPagIBIG);
    printf("- Income Tax:           -Php%.2f\n", e.lastIncomeTax);
    printf("-----------------------------------------\n");
    printf("TOTAL DEDUCTIONS:       -Php%.2f\n", totalDeduction);
    printf("NET SALARY:             Php%.2f\n", e.lastNetPay);
    printf("=========================================\n");

    printf("\nDo you want to print this salary slip to a text file? (Y/N): ");
    char choice = 'N';
    if (scanf(" %c", &choice) != 1) choice = 'N';
    clearInputBuffer();

    if (choice == 'Y' || choice == 'y') {
        printPaySlipToFile(&e);
    }
}

/* --- RECORD MANAGEMENT FUNCTIONS --- */

void viewEmployees(void) {
    system(CLEAR_COMMAND);
    if (employeeCount == 0) {
        printf("\nNo employees found.\n");
        return;
    }

    printf("\n=== Employee Records (%d) ===\n", employeeCount);
    printf("+----------+--------------------------------+------------------------+----------------+\n");
    printf("| %-8s | %-30s | %-22s | %-14s |\n", "ID", "Name", "Position", "Monthly Base");
    printf("+----------+--------------------------------+------------------------+----------------+\n");

    for (int i = 0; i < employeeCount; i++) {
        printf("| %-8d | %-30s | %-22s | Php%-11.2f |\n",
               employees[i].empID,
               employees[i].name,
               PositionNames[employees[i].position],
               employees[i].monthlySalary);
    }
    printf("+----------+--------------------------------+------------------------+----------------+\n");
}

void addEmployee(void) {
    system(CLEAR_COMMAND);
    if (employeeCount >= MAX_EMPLOYEES) {
        printf("\nEmployee list is full (max %d). Cannot add more employees.\n", MAX_EMPLOYEES);
        return;
    }

    Employee e;
    printf("\n=== Add New Employee ===\n");
    
    // Generate random 7-digit ID
    e.empID = generateEmployeeID();
    printf("Generated Employee ID: %d\n", e.empID);
    
    // Get name with duplicate validation
    char name[50];
    while (1) {
        getStringInput("\nEnter Full Name (max 48 chars): ", name, sizeof(name), 1);
        
        if (findEmployeeIndexByName(name) != -1) {
            printf("Error: Employee with name '%s' already exists!\n", name);
            printf("Please enter a different name.\n");
            continue;
        }
        break;
    }
    strcpy(e.name, name);

    printf("\nSELECT POSITION\n");
    for (int i = 0; i < NUM_POSITIONS; i++) {
        printf("%d. %s (Php%.2f per month)\n",
               i + 1, PositionNames[i], PositionMonthlySalaries[i]);
    }
    int posChoice = getIntInput("Choice: ", 1, NUM_POSITIONS);

    e.position = (PositionType)(posChoice - 1);
    e.monthlySalary = PositionMonthlySalaries[e.position];
    
    e.daysWorked = 0;

    // Initialize all calculation fields
    e.totalOvertimeHours = 0.0f;
    e.totalHoursWorked = 0.0f;
    e.lastOvertimePay = 0.0f;
    e.lastDailyRate = 0.0f;
    e.lastAbsentDeduct = 0.0f;
    e.lastGrossPay = 0.0f;
    e.lastNetPay = 0.0f;
    e.lastSSS = 0.0f;
    e.lastPhilHealth = 0.0f;
    e.lastPagIBIG = 0.0f;
    e.lastIncomeTax = 0.0f;

    printf("\n=== EMPLOYEE SUMMARY ===\n");
    printf("ID: %d\n", e.empID);
    printf("Name: %s\n", e.name);
    printf("Position: %s\n", PositionNames[e.position]);
    printf("Monthly Salary: Php%.2f\n", e.monthlySalary);
    
    printf("\nConfirm Add? (Y to confirm, any other key to cancel): ");
    char c = 'N';
    if (scanf(" %c", &c) != 1) c = 'N';
    clearInputBuffer();

    if (c == 'Y' || c == 'y') {
        employees[employeeCount++] = e;
        saveToFile();
        printf("\nEmployee '%s' added successfully with ID: %d.\n", e.name, e.empID);
    } else {
        printf("\nAdding cancelled. No changes made.\n");
    }
}

void updateEmployee(void) {
    system(CLEAR_COMMAND);
    if (employeeCount == 0) {
        printf("\nNo employees to update.\n");
        return;
    }
    printf("\n=== Update Employee ===\n");
    
    int id;
    printf("Enter Employee ID to update: ");
    if (scanf("%d", &id) != 1) {
        printf("Invalid ID format.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();
    
    if (id < 1000000 || id > 9999999) {
        printf("Invalid Employee ID. Must be 7 digits.\n");
        return;
    }
    
    int idx = findEmployeeIndexByID(id);

    if (idx == -1) {
        printf("\nEmployee with ID %d not found.\n", id);
        return;
    }

    Employee *e = &employees[idx];
    Employee old = *e;

    printf("\nCurrent Employee Details:\n");
    printf("Name: %s\n", e->name);
    printf("Position: %s (Php%.2f/month)\n", PositionNames[e->position], e->monthlySalary);
    printf("Days Worked (Last Summary): %d\n", e->daysWorked);

    printf("\nWhich field do you want to update?\n");
    printf("1) Name\n2) Position (and Monthly Salary)\n3) Back\n");
    int choice = getIntInput("Choice: ", 1, 3);

    if (choice == 3) {
        printf("\nUpdate cancelled.\n");
        return;
    }

    switch (choice) {
        case 1: {
            char newName[50];
            while (1) {
                getStringInput("\nEnter new full name: ", newName, sizeof(newName), 1);
                
                if (isNameDuplicate(newName, e->empID)) {
                    printf("Error: Employee with name '%s' already exists!\n", newName);
                    printf("Please enter a different name.\n");
                    continue;
                }
                break;
            }
            strcpy(e->name, newName);
            printf("Name updated to: %s\n", e->name);
            break;
        }
        case 2: {
            printf("\nSELECT NEW POSITION\n");
            for (int i = 0; i < NUM_POSITIONS; i++) {
                printf("%d. %s (Php%.2f per month)\n",
                       i + 1, PositionNames[i], PositionMonthlySalaries[i]);
            }
            int posChoice = getIntInput("Choice: ", 1, NUM_POSITIONS);
            e->position = (PositionType)(posChoice - 1);
            e->monthlySalary = PositionMonthlySalaries[e->position];
            printf("\nPosition updated to %s (Php%.2f/month).\n", PositionNames[e->position], e->monthlySalary);
            break;
        }
    }

    printf("\nConfirm update? (Y to confirm, any other key to cancel): ");
    char c = 'N';
    if (scanf(" %c", &c) != 1) c = 'N';
    clearInputBuffer();

    if (c == 'Y' || c == 'y') {
        saveToFile();
        printf("\nEmployee updated and saved successfully.\n");
    } else {
        *e = old;
        printf("\nUpdate cancelled; original record restored.\n");
    }
}

void removeEmployee(void) {
    system(CLEAR_COMMAND);
    if (employeeCount == 0) {
        printf("\nNo employees to delete.\n");
        return;
    }
    printf("\n=== Delete Employee ===\n");
    
    int id;
    printf("Enter Employee ID to delete: ");
    if (scanf("%d", &id) != 1) {
        printf("Invalid ID format.\n");
        clearInputBuffer();
        return;
    }
    clearInputBuffer();
    
    if (id < 1000000 || id > 9999999) {
        printf("Invalid Employee ID. Must be 7 digits.\n");
        return;
    }
    
    int idx = findEmployeeIndexByID(id);

    if (idx == -1) {
        printf("\nEmployee with ID %d not found.\n", id);
        return;
    }

    printf("\nEMPLOYEE TO DELETE:\n");
    printf("ID: %d\n", employees[idx].empID);
    printf("Name: %s\n", employees[idx].name);
    printf("Position: %s\n", PositionNames[employees[idx].position]);
    
    printf("\nARE YOU SURE YOU WANT TO DELETE THIS EMPLOYEE?\n");
    printf("This action cannot be undone! (Y to confirm): ");
    char c = 'N';
    if (scanf(" %c", &c) != 1) c = 'N';
    clearInputBuffer();

    if (c == 'Y' || c == 'y') {
        char deletedName[50];
        strcpy(deletedName, employees[idx].name);
        
        employees[idx] = employees[employeeCount - 1];
        --employeeCount;
        saveToFile();
        printf("\nEmployee '%s' deleted successfully.\n", deletedName);
    } else {
        printf("\nDeletion cancelled. No changes made.\n");
    }
}

void generateMonthlyAttendanceReport(void) {
    system(CLEAR_COMMAND);
    printf("\n=== MONTHLY ATTENDANCE REPORT ===\n");
    
    char monthYear[8];
    printf("Enter month and year (MM-YYYY): ");
    scanf("%7s", monthYear);
    clearInputBuffer();
    
    printf("\nReport for: %s\n", monthYear);
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
    printf("| %-7s | %-15s | %-12s | %-8s | %-8s | %-15s | %-15s |\n", 
           "ID", "Name", "Days Worked", "Absent", "Late", "Overtime Hours", "Total Hours");
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
    
    for (int i = 0; i < employeeCount; i++) {
        int daysWorked = 0;
        int lateCount = 0;
        float totalOvertime = 0.0;
        float totalHours = 0.0;
        
        for (int j = 0; j < attendanceCount; j++) {
            if (attendanceRecords[j].empID == employees[i].empID) {
                // Extract month-year from date (MM-YYYY format)
                char recordMonthYear[8] = {0};
                strncpy(recordMonthYear, attendanceRecords[j].date + 5, 2);
                recordMonthYear[2] = '-';
                strncpy(recordMonthYear + 3, attendanceRecords[j].date, 4);
                
                if (strcmp(recordMonthYear, monthYear) == 0) {
                    daysWorked++;
                    totalHours += attendanceRecords[j].hoursWorked;
                    totalOvertime += attendanceRecords[j].overtimeHours;
                    
                    if (attendanceRecords[j].isLate) {
                        lateCount++;
                    }
                }
            }
        }
        
        printf("| %-7d | %-15.15s | %-12d | %-8d | %-8d | %-15.2f | %-15.2f |\n",
               employees[i].empID,
               employees[i].name,
               daysWorked,
               STANDARD_WORKING_DAYS - daysWorked,
               lateCount,
               totalOvertime,
               totalHours);
    }
    printf("+---------+-----------------+--------------+----------+----------+-----------------+-----------------+\n");
}

/* --- ADMIN AND MAIN MENU FUNCTIONS --- */

int verifyPin(void) {
    int pin;
    int attempts = 3;
    
    while (attempts > 0) {
        printf("\nEnter Admin PIN (%d attempts remaining): ", attempts);
        if (scanf("%d", &pin) != 1) {
            clearInputBuffer();
            printf("Invalid input. Please enter numbers only.\n");
            attempts--;
            continue;
        }
        clearInputBuffer();
        if (pin == ADMIN_PIN) {
            printf("\nLogin successful.\n");
            return 1;
        } else {
            printf("Login failed.\n");
            attempts--;
        }
    }
    
    printf("\nToo many failed attempts. Access denied.\n");
    return 0;
}

void adminMenu(void) {
    system(CLEAR_COMMAND);
    int choice;
    do {
        printf("\nADMIN CONTROL PANEL\n");
        printf("\n--- RECORD MANAGEMENT ---\n");
        printf("1. View All Employee Records\n");
        printf("2. Add New Employee\n");
        printf("3. Update Employee Detail\n");
        printf("4. Remove Employee\n");
        printf("\n--- ATTENDANCE MANAGEMENT ---\n");
        printf("5. Record Time-In\n");
        printf("6. Calculate Attendance Summary\n");
        printf("7. Generate Monthly Attendance Report\n");
        printf("\n--- SALARY COMPUTATION ---\n");
        printf("8. Calculate & View Monthly Salary Computation\n");
        printf("9. View Individual Salary Slip\n");
        printf("\n10. BACK TO MAIN MENU\n");
        printf("\nChoice: ");

        if (scanf("%d", &choice) != 1) {
            printf("\nInvalid input. Please enter a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1: viewEmployees(); break;
            case 2: addEmployee(); break;
            case 3: updateEmployee(); break;
            case 4: removeEmployee(); break;
            case 5: recordTimeIn(); break;
            case 6: calculateAttendanceSummary(); break;
            case 7: generateMonthlyAttendanceReport(); break;
            case 8: calculateAndDisplaySalary(); break;
            case 9: {
                int id = getIntInput("\nEnter Employee ID for Salary Slip: ", 1000000, 9999999);
                displayEmployeeSalarySlip(id);
                break;
            }
            case 10: printf("\nLogging out of Admin.\n"); break;
            default: printf("\nInvalid choice. Please select 1-10.\n");
        }
        if (choice != 10) pressEnterToContinue();
    } while (choice != 10);
}

void mainMenu(void) {
    int choice;
    do {
        system(CLEAR_COMMAND);
        printf("\nEMPLOYEE RECORD MANAGEMENT SYSTEM\n");
        printf("\n1. ADMIN LOGIN\n");
        printf("\n2. VIEW ALL EMPLOYEE RECORDS\n");
        printf("\n3. EXIT SYSTEM\n");
        printf("\nChoice: ");

        if (scanf("%d", &choice) != 1) {
            printf("\nInvalid input. Please enter a number.\n");
            clearInputBuffer();
            continue;
        }
        clearInputBuffer();

        switch (choice) {
            case 1:
                if (verifyPin()) adminMenu();
                else pressEnterToContinue();
                break;
            case 2:
                viewEmployees();
                pressEnterToContinue();
                break;
            case 3:
                printf("\nExiting system. All changes saved.\n");
                break;
            default:
                printf("\nInvalid choice. Please select 1-3.\n");
                pressEnterToContinue();
        }
    } while (choice != 3);
}

int main(void) {
    printf("Loading employee data...\n");
    loadFromFile();
    loadAttendanceFromFile();
    mainMenu();
    return 0;
}