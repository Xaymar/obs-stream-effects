const HTTPS = require("https");
const FS = require("fs/promises");
const CHILD_PROCESS = require("child_process");

let project_id = 343435
let project_auth_key = "";

function query_git() {
	// git shortlog -sn --all | sed -E "s/^\W+[0-9]+\W+//"
	return (async () => {
		let result = [];

		let git = new Promise((resolve, reject) => {
			try {
				const child = CHILD_PROCESS.spawn('git', ['shortlog', '-sn', '--all']);

				let sout = "";
				child.stdout.setEncoding('utf8');
				child.stdout.on('data', (data) => {
					sout += data;
				});

				let serr = "";
				child.stderr.setEncoding('utf8');
				child.stderr.on('data', (data) => {
					serr += data;
				});

				child.on('close', (code) => {
					if (code != 0) {
						reject([code, serr]);
					} else {
						resolve(sout);
					}
				});
			} catch (e) {
				reject(e);
			}
		});

		git.then((result) => {
			console.log(result);
			let lines = result.split(/(\r\n|\n|\r)/);
			for (let line of lines) {
				console.log(line);
			}
		}, (error) => {
			throw error;
		});
		return [];
	})();
}

function query_crowdin(project_id, project_auth_key) {
	const limit = 100;
	function _query_page(page) {
		return new Promise((resolve, reject) => {
			try {
				let query = {
					protocol: "https:",
					host: "crowdin.com",
					path: `/api/v2/projects/${project_id}/members?limit=${limit}&offset=${page * limit}`,
					method: "GET",
					headers: {
						"accept": "application/json",
						"authorization": `Bearer ${project_auth_key}`
					}
				};
				let req = HTTPS.request(query, (res) => {
					let data = "";
					res.setEncoding('utf8');
					res.on('data', (chunk) => {
						data += chunk;
					});
					res.on('end', () => {
						res.data = data;
						resolve(res);
					});
				});
				req.end();
			} catch (e) {
				reject(e);
			}
		});
	}

	return (async () => {
		let page_is_last = false;
		let page = 0;

		let users = {};

		while (!page_is_last) {
			let res = await _query_page(page++);
			if (res.statusCode == 200) {
				let json = JSON.parse(res.data);
				let count = json.data.length;
				for (let user of json.data) {
					// Don't credit blocked users for work not done.
					if (user.data.role == 'blocked') {
						continue;
					}

					let key = (user.data.fullName !== null) && (user.data.fullName !== undefined) && (user.data.fullName.length > 0) ? user.data.fullName : user.data.username;

					users[key] = `https://crowdin.com/profile/${user.data.username}`;
				}

				page_is_last = (count < limit);
			} else {
				throw new Error(JSON.parse(res.data));
				page_is_last = true;
			}
		}

		return users;
	})();
}

query_git().then((result) => {
	const keys_sorted = Object.keys(result).sort();

	let markdown = "";
	let json = [];

	for (let key of keys_sorted) {
		if (markdown.length > 0)
			markdown += ", ";
		markdown += `[${key}](${result[key]})`;

		json.push({
			name: key,
			url: result[key]
		})
	}

	// Write Markdown file
	FS.writeFile("contributor.md", markdown, {
		encoding: 'utf8',
	});

	FS.writeFile("contributor.json", JSON.stringify(json, undefined, '\t'), {
		encoding: 'utf8',
	});
}, (error) => {
	console.error(error);
});
query_crowdin(project_id, project_auth_key).then((result) => {
	const keys_sorted = Object.keys(result).sort();

	let markdown = "";
	let json = [];
	let cpp = "";

	for (let key of keys_sorted) {
		if (markdown.length > 0)
			markdown += ", ";
		markdown += `[${key}](${result[key]})`;

		json.push({
			name: key,
			url: result[key]
		})

		if (cpp.length > 0) cpp += "\n";
		cpp += `\tstreamfx::ui::about::entry{"${key}", streamfx::ui::about::role_type::TRANSLATOR, "",
		"${result[key]}"}, // ${result[key]}`
	}

	// Write Markdown file
	FS.writeFile("translators.md", markdown, {
		encoding: 'utf8',
	});

	FS.writeFile("translators.json", JSON.stringify(json, undefined, '\t'), {
		encoding: 'utf8',
	});

	FS.writeFile("translators.cpp", cpp, {
		encoding: 'utf8',
	});
}, (error) => {
	console.error(error);
});
