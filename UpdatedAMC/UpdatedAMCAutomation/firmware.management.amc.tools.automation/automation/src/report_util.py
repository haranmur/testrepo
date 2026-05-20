import re
import os
import datetime
import serial_util
import setup_env
setup_env.setup_environment()

# --- ANSI to HTML conversion utility ---


def ansi_to_html(text):
    ansi_colors = {
        '30': 'black', '31': 'red', '32': 'green', '33': 'yellow', '34': 'blue',
        '35': 'magenta', '36': 'cyan', '37': 'white',
        '90': 'gray', '91': 'lightcoral', '92': 'lightgreen', '93': 'khaki',
        '94': 'lightblue', '95': 'violet', '96': 'lightcyan', '97': 'white'
    }
    # Match all ANSI escape sequences (not just color codes)
    ansi_escape = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')

    def repl(match):
        codes = match.group(0)[2:-1].split(';')
        color = None
        for code in codes:
            if code in ansi_colors:
                color = ansi_colors[code]
        if color:
            return f'<span style="color: {color}">'  # open span
        elif '0' in codes:
            return '</span>'  # reset/close
        return ''
    # Remove all ANSI codes and replace with HTML
    html = ansi_escape.sub(repl, text)
    # Remove any remaining raw escape codes (just in case)
    html = re.sub(r'\x1b\[[0-9;?]*[A-Za-z]', '', html)
    return html


results = []


def get_report_dt_str():
    # Always use the serial_util's session datetime string
    return serial_util.get_serial_log_dt_str()


def get_console_log_filename():
    dt_str = get_report_dt_str()
    results_dir = "/results"
    if not os.path.exists(results_dir):
        results_dir = os.path.join(os.getcwd(), "results")
    os.makedirs(results_dir, exist_ok=True)
    return os.path.join(results_dir, f"summary_result_{dt_str}.txt")


def add_result(test, status, details):
    idx = len(results) + 1
    results.append({"test": test, "status": status,
                   "details": details, "number": idx})
    log_line = f"[{idx}] {test}: {status} - {details}"
    print(log_line)
    # Also write to the session console log file
    with open(get_console_log_filename(), "a", encoding="utf-8") as f:
        f.write(log_line + "\n")


def generate_html_report(results_arg=None):
    if results_arg is None:
        results_to_use = results
    else:
        results_to_use = results_arg
    dt_str = get_report_dt_str()
    results_dir = "/results"
    if not os.path.exists(results_dir):
        results_dir = os.path.join(os.getcwd(), "results")
    os.makedirs(results_dir, exist_ok=True)
    console_log_name = f"console_log_{dt_str}.txt"
    summary_log_name = f"summary_result_{dt_str}.txt"
    bmc_log_basename = f"bmc_ssh_test_log_{dt_str}.txt"
    report = f"""
    <html>
    <head>
        <meta charset='UTF-8'>
        <title>AMC Test Report</title>
        <style>
            body {{ font-family: Arial, sans-serif; }}
            table {{ width: 100%; border-collapse: collapse; }}
            th, td {{ border: 1px solid #ddd; padding: 8px; }}
            th {{ background-color: #f2f2f2; }}
            .success {{ color: green; }}
            .failure {{ color: red; }}
            pre.serial-log-html {{ background: #222; color: #eee; padding: 10px; border-radius: 5px; overflow-x: auto; }}
        </style>
        <script>
        function toggleSerialLog() {{
            var e = document.getElementById('serial-log-block');
            var btn = document.getElementById('serial-log-toggle');
            if (e.style.display === 'none') {{
                e.style.display = 'block';
                btn.textContent = 'Hide Serial Log';
            }} else {{
                e.style.display = 'none';
                btn.textContent = 'Show Serial Log';
            }}
        }}
        </script>
    </head>
    <body>
        <h1>AMC Test Report</h1>
        <p>Generated on: {datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')}</p>
        <p>
            <b>Related logs:</b>
            <a href='{console_log_name}' target='_blank'>Console Log (.txt)</a> |
            <a href='{summary_log_name}' target='_blank'>Summary Results (.txt)</a> |
            <a href='{bmc_log_basename}' target='_blank'>BMC SSH Test Log(.txt)</a>
        </p>
    """
    # Calculate summary and build summary table (keep first, remove second)
    summary = {}
    for result in results_to_use:
        module = result['test'].split()[0]
        if module not in summary:
            summary[module] = {'Success': 0, 'Failure': 0, 'numbers': []}
        summary[module][result['status']] += 1
        summary[module]['numbers'].append(result['number'])

    # Prepare module list for columns
    module_names = list(summary.keys())
    # Header row with module names
    report += "<table>"
    report += "<tr><th> Modules </th>"
    for module in module_names:
        report += f"<th>{module}</th>"
    report += "</tr>"

    # Single row for results with colored S/F and light green if no failures
    report += "<tr><td>Results [✅/❌]</td>"
    for module in module_names:
        s = summary[module]['Success']
        f = summary[module]['Failure']
        if f == 0:
            cell_style = " style='background-color: #d4edda;'"
        elif f > 0:
            cell_style = " style='background-color: #f8d7da;'"
        else:
            cell_style = ""
        report += f"<td{cell_style}><span class='success'>{s}</span>/<span class='failure'>{f}</span></td>"
    report += "</tr>"
    report += "</table>"

    # Embed BMC SSH log with toggle, after summary table
    bmc_log_path = os.path.join(results_dir, bmc_log_basename)
    bmc_log_html = ""
    if os.path.exists(bmc_log_path):
        with open(bmc_log_path, "r", encoding="utf-8") as f:
            bmc_log_raw = f.read()
        import html
        bmc_log_html = html.escape(bmc_log_raw)
        report += (
            "<div style='margin-top:10px;'>"
            "<b>BMC SSH Test Log:</b> "
            "<a href=\"#\" id=\"bmc-log-toggle\" onclick=\"var e=document.getElementById('bmc-log-block');var btn=document.getElementById('bmc-log-toggle');if(e.style.display==='none'){e.style.display='block';btn.textContent='Hide BMC SSH Log';}else{e.style.display='none';btn.textContent='Show BMC SSH Log';}return false;\">Show BMC SSH Log</a> "
            "<div id='bmc-log-block' style='display:none; margin-top:10px;'><pre class='serial-log-html'>" +
            bmc_log_html + "</pre></div></div>"
        )

    # Embed serial log with ANSI color as HTML, but only show on click (after summary)
    serial_log_path = os.path.join(results_dir, console_log_name)
    serial_log_html = ""
    if os.path.exists(serial_log_path):
        with open(serial_log_path, "r", encoding="utf-8") as f:
            serial_log_raw = f.read()
        serial_log_html = ansi_to_html(serial_log_raw)
        report += ("<div style='margin-top:10px;'><b>Serial Log (with color):</b> "
                   "<a href=\"#\" id=\"serial-log-toggle\" onclick=\"toggleSerialLog(); return false;\">Show Serial Log</a> "
                   "<div id='serial-log-block' style='display:none; margin-top:10px;'><pre class='serial-log-html'>" + serial_log_html + "</pre></div></div>")

    # (Removed duplicate summary table block)

    # Add detailed results
    for module in summary.keys():
        report += """
        <h2>{} Module</h2>
        <table>
            <tr>
                <th style=\"width: 10%;\">#</th>
                <th style=\"width: 25%;\">Test</th>
                <th style=\"width: 15%;\">Status</th>
                <th style=\"width: 50%;\">Details</th>
            </tr>
        """.format(module)

        for result in results_to_use:
            if result['test'].split()[0] == module:
                status_class = "success" if result['status'] == "Success" else "failure"
                report += """
                    <tr>
                        <td style=\"width: 10%;\">{}</td>
                        <td style=\"width: 25%;\">{}</td>
                        <td class=\"{}\" style=\"width: 15%;\">{}</td>
                        <td style=\"width: 50%;\">{}</td>
                    </tr>
                """.format(result['number'], result['test'], status_class, result['status'], result['details'])

        report += """
        </table>
        """

    report += """
        </body>
        </html>
    """

    dt_str = get_report_dt_str()
    report_filename = f"amc_test_report_{dt_str}.html"
    results_dir = "/results"

    # Check if /results directory exists
    if not os.path.exists(results_dir):
        # If not, create results directory in the current working directory
        results_dir = os.path.join(os.getcwd(), "results")

    os.makedirs(results_dir, exist_ok=True)
    report_filepath = os.path.join(results_dir, report_filename)
    with open(report_filepath, "w", encoding="utf-8") as file:
        file.write(report)

    # Prepare file:// link for local viewing
    report_file_url = f"file://{report_filepath}"
    link_message = f"HTML report generated: {report_filepath}\nOpen in browser: {report_file_url}"
    print(link_message)

    # Also append the link to the console log and summary results log
    for log_name in [console_log_name, summary_log_name]:
        log_path = os.path.join(results_dir, log_name)
        try:
            with open(log_path, "a", encoding="utf-8") as log_file:
                log_file.write("\n" + link_message + "\n")
        except Exception as e:
            print(
                f"Warning: Could not write HTML report link to {log_path}: {e}")

    # Clean ANSI characters from console log file at the end
    console_log_path = os.path.join(results_dir, console_log_name)
    if os.path.exists(console_log_path):
        try:
            with open(console_log_path, "r", encoding="utf-8") as f:
                content = f.read()

            # Remove ANSI escape sequences and control characters
            ansi_escape = re.compile(r'\x1b\[[0-9;?]*[A-Za-z]')
            cleaned_content = ansi_escape.sub('', content)
            # Remove other control characters but keep printable chars and newlines/tabs
            cleaned_content = re.sub(
                r'[\x00-\x08\x0B\x0C\x0E-\x1F\x7F-\x9F]', '', cleaned_content)

            with open(console_log_path, "w", encoding="utf-8") as f:
                f.write(cleaned_content)

            print(
                f"Cleaned ANSI characters from console log: {console_log_path}")
        except Exception as e:
            print(
                f"Warning: Could not clean console log file {console_log_path}: {e}")
