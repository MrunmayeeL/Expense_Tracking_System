# Expense Tracking System — January 2025

A C-based application for managing and tracking family finances using nested B+ trees, linked lists, and file persistence.

## Overview

This Expense Tracking System helps users manage individual and family expenses with persistent data storage. It supports features like adding, deleting, and updating families, individuals, and expenses, along with analytical features such as retrieving highest expense days or category-wise breakdowns.

Built entirely in C, this application simulates a basic database using:
- Nested B+ trees for efficient record management.
- Structures of arrays and linked lists for data organization.
- File handling for persistent storage.

## Features

- Persistent storage using `expense_data.txt`
- Family management: create, update, and delete family records
- Individual management: track income and expense history of members
- Expense management: categorized expenses with filtering by time and ID range
- Analytics:
  - Total expenses by an individual
  - Category-wise expense breakdown
  - Highest expense day in a family
  - Expenses filtered by date range or ID range

## Data Structures

- B+ Trees (3 levels):
  - Family → Individual → Expense
- Enum-based categories: Rent, Utility, Grocery, Stationary, Leisure
- Custom file I/O format for reading/writing data

## Build and Run

### Requirements
- A C compiler (e.g., `gcc`)
- Standard C library support

### Compile
```bash
gcc -o expense_tracker Expense_tracking.c
