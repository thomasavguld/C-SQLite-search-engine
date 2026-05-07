# Local Search Engine in C + SQLite

A local document ingestion, indexing and search engine written in C using SQLite.

The project:

1. Reads JSON documents from a local folder
2. Extracts metadata and text
3. Stores documents in SQLite
4. Builds a trigram/n-gram search index
5. Allows interactive local search through a CLI

---

# Features

- JSON ingestion pipeline
- SQLite-based storage
- Author/document relational model
- Trigram/n-gram indexing
- Local CLI search
- Batch ingestion and indexing
- Metrics and throughput logging

---

# Project Structure

```text
project/
│
├── build/                 # Compiled executable
├── db/                    # SQLite database
├── external/              # External libraries
│   └── yyjson/
├── include/               # Header files
├── src/                   # Source code
├── warehouse/             # JSON files go here
├── Makefile
└── README.md
```

---

# Requirements

You need:

- macOS or Linux
- clang compiler
- make
- SQLite3
- terminal access

---

# Step 1 — Install Required Tools

## macOS

Open Terminal and install Xcode Command Line Tools:

```bash
xcode-select --install
```

Check that clang exists:

```bash
clang --version
```

Check that make exists:

```bash
make --version
```

Check SQLite:

```bash
sqlite3 --version
```

If SQLite is missing:

```bash
brew install sqlite
```

---

## Ubuntu / Debian Linux

Open terminal:

```bash
sudo apt update
sudo apt install build-essential sqlite3 libsqlite3-dev
```

Verify:

```bash
clang --version
make --version
sqlite3 --version
```

---

# Step 2 — Clone the Repository

Choose a location for the project.

Example:

```bash
cd ~/Documents
```

Clone:

```bash
git clone <YOUR_REPO_URL>
```

Go into the project:

```bash
cd <PROJECT_FOLDER>
```

Example:

```bash
cd local-search-engine
```

---

# Step 3 — Download the JSON Dataset

This project expects JSON files from this dataset:

Elsevier OA CC-BY corpus:

https://elsevier.digitalcommonsdata.com/datasets/zm33cdndxs/2

Download the dataset ZIP file manually through the browser.

---

# Step 4 — Extract the Dataset

After downloading:

Locate the ZIP file.

Extract it.

Example:

```bash
unzip dataset.zip
```

You should now have many `.json` files.

---

# Step 5 — Move JSON Files into the Warehouse Folder

Inside the project there is a folder called:

```text
warehouse/
```

Move all JSON files into that folder.

Example:

```bash
mv ~/Downloads/dataset/*.json warehouse/
```

Verify:

```bash
ls warehouse
```

You should see many JSON files listed.

Example:

```text
0001.json
0002.json
0003.json
...
```

---

# Step 6 — Build the Project

From the project root:

```bash
make
```

You should see compilation output. Or maybe not?

If successful, the executable will be created here:

```text
build/search
```

---

# Step 7 — Run the Program

Run:

```bash
make run
```

or:

```bash
./build/search
```

---

# Step 8 — Ingest Documents

The program will ask whether you want to ingest documents.

Press:

```text
Enter
```

The system will:

1. Traverse the warehouse folder
2. Parse JSON files
3. Insert documents into SQLite
4. Insert authors and relations
5. Commit batches
6. Print throughput metrics

Depending on dataset size this may take:

- minutes
- or longer for very large datasets

---

# Step 9 — Build the Search Index

After ingestion the program can build the trigram index.

This step:

1. Reads documents from SQLite
2. Generates trigrams
3. Stores grams in the `ngrams` table

This may also take time depending on dataset size.

---

# Step 10 — Start Searching

After indexing completes the search REPL starts.

Example:

```text
search>
```

Type a query:

```text
cancer diagnosis
```

Press Enter.

The engine will:

1. Convert query into trigrams
2. Match grams against the inverted index
3. Rank documents by hit count
4. Display results

---

# SQLite Database

The SQLite database is stored here:

```text
db/c_search.db
```

You can inspect it manually:

```bash
sqlite3 db/c_search.db
```

Example queries:

```sql
.tables
```

```sql
SELECT COUNT(*) FROM documents;
```

```sql
SELECT title FROM documents LIMIT 5;
```

Exit SQLite:

```sql
.quit
```

---

# Rebuilding from Scratch

To delete build artifacts and database:

```bash
make clean
```

If needed, manually remove the database:

```bash
rm db/c_search.db
```

Then rebuild:

```bash
make
```

Run again:

```bash
./build/search
```

Or clean and run:

```bash
make clean && make run
```
---

# Common Problems

## 1. "Warehouse directory is empty"

Cause:

No JSON files exist in:

```text
warehouse/
```

Fix:

Move JSON files into the warehouse directory.

Verify:

```bash
ls warehouse
```

---

## 2. "sqlite3.h file not found"

Cause:

SQLite development headers are missing.

macOS:

```bash
brew install sqlite
```

Ubuntu:

```bash
sudo apt install libsqlite3-dev
```

---

## 3. "yyjson.h file not found"

Cause:

Wrong folder structure.

Verify this exists:

```text
external/yyjson/src/yyjson.h
```

---

## 4. Permission denied

Make executable:

```bash
chmod +x build/search
```

---

## 5. Build fails after changing code

Clean and rebuild:

```bash
make clean
make
```

---

# How the Search Engine Works

Simplified pipeline:

```text
JSON files
    ↓
yyjson parsing
    ↓
SQLite staging
    ↓
documents/authors tables
    ↓
trigram generation
    ↓
ngrams table
    ↓
CLI search
```

---

# Technologies Used

- C
- SQLite3
- yyjson
- Make
- clang

---

# Current Limitations

- Single-process indexing
- SQLite single-writer limitation
- No BM25 ranking
- No stemming/tokenization
- Large trigram indexes
- No distributed search

---

# Future Improvements

Possible future work:

- BM25 ranking
- parallel ingestion
- stemming
- tokenization
- web API
- fuzzy ranking
- compressed indexes
- distributed indexing

---

# References

SQLite:

https://www.sqlite.org/docs.html

yyjson:

https://github.com/ibireme/yyjson

Introduction to Information Retrieval:

https://nlp.stanford.edu/IR-book/


---

# Open Source License

This project is released under the GNU General Public License v3.0 (GPL-3.0).

This means:

- You are free to use, study, modify and distribute the software
- Any modified version that is distributed must also remain open source
- Source code must remain available under the same license terms

The project is therefore considered copyleft software.

Full license text:

https://www.gnu.org/licenses/gpl-3.0.en.html

---

# Repository

Git repository:

```text
https://github.com/thomasavguld/c-search-engine.git
```

Clone example:

```bash
git clone https://github.com/<YOUR_USERNAME>/<YOUR_REPOSITORY>.git
```

---

# Author

Created by:

```text
Thomas av Guld - 2026

```

---

# Contributing

Contributions, bug reports and pull requests are welcome.

Recommended workflow:

1. Fork the repository
2. Create a feature branch
3. Commit changes
4. Open a pull request

Example:

```bash
git checkout -b feature/my-feature
```

---

# Disclaimer

This project was developed primarily as:

- a systems programming project
- a learning project in information retrieval
- an experimental local search engine

The software is provided without warranty.
