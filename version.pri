GIT_VERSION = $$system("bash -c \"git --version\"")

!isEmpty(GIT_VERSION) {
	message($$GIT_VERSION)
	message("Git Information Available")

	BRANCH_COUNT = $$system("bash -c \"cd \'$$_PRO_FILE_PWD_\'; git branch -a | wc -l\"")

	greaterThan(BRANCH_COUNT, 1) {
		GIT_HEAD = $$system("bash -c \"cd \'$$_PRO_FILE_PWD_\'; git rev-parse HEAD\"")
		GIT_BRANCH = $$system("bash -c \"cd \'$$_PRO_FILE_PWD_\'; git branch | cut -c 3-\"")
		GIT_REVISION = $$system("bash -c \"cd \'$$_PRO_FILE_PWD_\'; git rev-list HEAD | wc -l\"")
		GIT_AUTHORS = $$system("bash -c \"cd \'$$_PRO_FILE_PWD_\'; git log --format=\'%aN<%aE>\' | sort -u | sed \':a;N;$!ba;s/\\n/,/g\' -\"")
		message("$$GIT_BRANCH $$GIT_HEAD")
		message("Revision $$GIT_REVISION")
		!isEmpty(GIT_AUTHORS) {
			message("Authors $$GIT_AUTHORS")
			DEFINES += $$quote(GIT_AUTHORS=\'\"$$GIT_AUTHORS\"\')
		}

		!isEmpty(GIT_HEAD):DEFINES += $$quote(GIT_HEAD=\'\"$$GIT_HEAD\"\')
		!isEmpty(GIT_BRANCH):DEFINES += $$quote(GIT_BRANCH=\'\"$$GIT_BRANCH\"\')

		!isEmpty(GIT_REVISION) {
			DEFINES += $$quote(GIT_REVISION=\'\"$$GIT_REVISION\"\')
			DEFINES += $$quote(RAW_GIT_REVISION=\'$$GIT_REVISION\')
			VER_PAT = $$GIT_REVISION
		}
	} else {
		message("Source not connected to git repo,")
		message("Or system command does not support pipelines.")
		message("Debugging tools will not be compiled for this build...")
	}
} else: warning("Git was not detected on your system.")

isEmpty(VER_MAJ): VER_MAJ = 0
isEmpty(VER_MIN): VER_MIN = 1

DEFINES += $$quote(VER_MAJ=\'\"$$VER_MAJ\"\')
DEFINES += $$quote(VER_MIN=\'\"$$VER_MIN\"\')
DEFINES += $$quote(RAW_VER_MAJ=\'$$VER_MAJ\')
DEFINES += $$quote(RAW_VER_MIN=\'$$VER_MIN\')
