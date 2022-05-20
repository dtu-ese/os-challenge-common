import sys
import re
import os

def gen_group():
	print("Generating a new group...")
	print()

	group = input("Group name: ")

	pin = input("PIN (must be positive 6-digit integer): ")
	if not re.match('^[0-9]+$', pin) or not len(pin)==6 or pin=='123456':
		print("Error: PIN must be positive 6-digit integer. It cannot be 123456.")
		exit(1)

	repo = input ("Group Repository URL (e.g. https://github.com/XXXX/os-challenge-%s.git): " % group)


	num = input("Number of group members: ")
	if not re.match('^[0-9]+$', num) or int(num)==0 or int(num)>=10:
		print("Error: Must be small positive integer.")
		exit(1)

	out = ','.join([group, pin, repo, num])


	for i in range(int(num)):
		name = input("[Student %d] Full Name (as in student card): " % i)
		email = input("[Student %d] DTU Email: " % i)
		gitname = input("[Student %d] Git Username: " % i)
		out = out + ',' + ','.join([name, email, gitname])

	print()
	print("Your group submission string is: ")
	print(out)

	file = group + ".csv"
	with open(file, 'w') as f:
		f.write(out + '\n')
	
	print()
	print("Submission file generated: %s" % file)


def gen_submission(s):
	print("Generating a %s submission..." % s)
	print()

	groupfile = input("Path to your group csv file (e.g. xefa.csv): ")
	
	if not os.path.exists(groupfile):
		print('Error: file does not exist.')
		exit(1)

	with open(groupfile, 'r') as f:
		entry = f.readline()
		group = entry.split(',')[0]
		print("Group Name: %s" % group)

	githash = input ("Submission Git Hash (e.g. f54c4e538483ecd8bbf5482a986bc13b12639547): ")

	out = ','.join([group, githash])

	print()
	print("Your %s submission string is: " % s)
	print(out)

	file = group + "-" + s + ".csv"
	with open(file, 'w') as f:
		f.write(out + '\n')
	
	print()
	print("Submission file generated: %s" % file)


print("**************************")
print("* DTU 02159 OS Challenge *")
print("**************************")
print()

if len(sys.argv)!=2:
	print("Error: Use group, milestone or final as input argument.")
	exit(1)

if sys.argv[1] == 'group':
	gen_group()
elif sys.argv[1] == 'milestone':
	gen_submission('milestone')
elif sys.argv[1] == 'final':
	gen_submission('final')
else:
	print("Error: Use group, milestone or final as input argument.")
	exit(1)