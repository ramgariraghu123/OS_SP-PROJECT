#ifndef CD_BUILTIN_H
#define CD_BUILTIN_H

int builtin_cd(int argc, char **argv);
int builtin_pwd(int argc, char **argv);
void init_pwd_environment(void);

#endif /* CD_BUILTIN_H */
