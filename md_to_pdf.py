from fpdf import FPDF
import os

class PDFReport(FPDF):
    def header(self):
        self.set_font('Helvetica', 'B', 14)
        self.cell(0, 10, 'Informe de Validación randu_plus', new_x="LMARGIN", new_y="NEXT", align='C')
        self.line(10, self.get_y(), 200, self.get_y())
        self.ln(5)

    def footer(self):
        self.set_y(-15)
        self.set_font('Helvetica', 'I', 8)
        self.cell(0, 10, f'Página {self.page_no()}/{{nb}}', align='C')

pdf = PDFReport()
pdf.alias_nb_pages()
pdf.add_page()
pdf.set_auto_page_break(auto=True, margin=20)

with open('VALIDATION_REPORT.md', 'r') as f:
    md = f.read()

lines = md.split('\n')
in_table = False
table_rows = []
col_widths = [60, 80, 20]

def flush_table():
    global in_table, table_rows
    if not table_rows:
        in_table = False
        return
    for i, row in enumerate(table_rows):
        if i == 0:
            pdf.set_font('Helvetica', 'B', 9)
        else:
            pdf.set_font('Helvetica', '', 9)
        for j, cell in enumerate(row):
            w = col_widths[j] if j < len(col_widths) else 40
            cell = cell.replace('\u2705', '[PASS]').replace('\u274c', '[FAIL]')
            pdf.cell(w, 6, cell[:30], 1, new_x="RIGHT", new_y="TOP", align='L')
        pdf.ln()
    pdf.ln(4)
    in_table = False
    table_rows = []

for line in lines:
    stripped = line.strip()
    
    if stripped.startswith('|') and stripped.endswith('|'):
        cells = [c.strip() for c in stripped[1:-1].split('|')]
        if not in_table:
            in_table = True
            table_rows = [cells]
        else:
            if all(c.startswith('-') for c in cells):
                continue
            table_rows.append(cells)
    else:
        if in_table:
            flush_table()
        
        if stripped:
            if stripped.startswith('### '):
                pdf.set_font('Helvetica', 'B', 12)
                pdf.cell(0, 8, stripped[4:], new_x="LMARGIN", new_y="NEXT")
                pdf.ln(2)
            elif stripped.startswith('## '):
                pdf.set_font('Helvetica', 'B', 14)
                pdf.cell(0, 10, stripped[3:], new_x="LMARGIN", new_y="NEXT")
                pdf.ln(3)
            elif stripped.startswith('# '):
                pdf.set_font('Helvetica', 'B', 16)
                pdf.cell(0, 12, stripped[2:], new_x="LMARGIN", new_y="NEXT", align='C')
                pdf.ln(5)
            elif stripped.startswith('**') and stripped.endswith('**'):
                pdf.set_font('Helvetica', 'B', 10)
                pdf.multi_cell(0, 5, stripped[2:-2])
                pdf.ln(1)
            elif '**:' in stripped:
                parts = stripped.split('**:', 1)
                pdf.set_font('Helvetica', 'B', 10)
                pdf.write(5, parts[0] + '**:')
                pdf.set_font('Helvetica', '', 10)
                pdf.write(5, parts[1] if len(parts) > 1 else '')
                pdf.ln(5)
            else:
                pdf.set_font('Helvetica', '', 10)
                pdf.multi_cell(0, 5, stripped)
                pdf.ln(1)

if in_table:
    flush_table()

output_path = 'VALIDATION_REPORT.pdf'
pdf.output(output_path)
print(f"PDF generado: {output_path}")